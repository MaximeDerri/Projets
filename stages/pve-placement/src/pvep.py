#!/usr/bin/env python3

import sys
import time
import random
import paramiko
from proxmoxer import ProxmoxAPI
from proxmoxer.tools import Tasks
from node import Node
from vm import *
from config import *
from storage import *
from migration import *
from tools import *


sep = "============================="


#move item (in list) to the top of the list (index=0)
def to_top(item, list) -> int:
	if list == None:
		return -1

	list.remove(item)
	list.insert(0, item)
	return 0


def count_vms(node_list, vm_list) -> int:
	if node_list == None or vm_list == None:
		return -1

	try:
		for node in node_list:
			node.nr_vm = len(vm_list[node.name])
	except KeyError:
		print('ERROR: Unknown key - count_vms')
		exit(1)

	return 0


#None = algorithm should stop
#Node = algorithm should continue
def placement_balance_node(node_list, config) -> int | Node:
	#if we find a node where the balance exceeds the average + scale%, the algorithm will work with
	#if a such node doesn't exist, if we found a node under average - scale%, the algorithm will work with the node
	#that has the greatest balance
	#otherwise, the algorithm should stop
	if node_list == None or config == None:
		return -1

	default = None
	use_default = False
	scale = config.scale / 100
	average = 0
	high = 0
	low = 0

	for node in node_list:
		average += node.balance
	average /= len(node_list)
	high = average * (1 + scale)	#1.x
	low = average * scale			#0.x

	for node in node_list:
		if node.name not in config.vm_from or node.nr_vm <= 0:
			continue

		if node.balance > high:
			return node
		elif node.balance < low:
			use_default = True
		
		if (default == None or node.balance > default.balance):
			default = node
	
	if use_default:

		return default
	else:
		return None


def select_next_node_with_vm(node_list, vm_list) -> bool:
	if node_list == None or vm_list == None:
		return False

	for i in range(len(node_list)):
		if len(vm_list[node_list[i].name]) > 0:
			if i == 0:
				return True
			else:
				tmp = node_list[i]
				node_list[i] = node_list[0]
				node_list[0] = tmp
				return True

	return False


#if migrate == True, this function will update the src and tgt node objects and return a Migration object
#else, this function will return the number of fitable storages
#this function can do 2 different things because the code is nearly the same
def count_fitable_storages_or_setup_migration(src, tgt, vm, config, migrate) -> int | Migration:
	if src == None or tgt == None or vm == None or config == None:
		return -1

	count = 0
	map_src = dict()
	map_tgt = dict()
	migration = Migration()
	src_st = src.storage_list
	tgt_st = tgt.storage_list

	map_tgt['lvm'] = dict()
	map_tgt['lvmthin'] = dict()
	map_tgt['zfs'] = dict()
	map_tgt['dir'] = dict()
	map_tgt['btrfs'] = dict()

	for s in tgt_st:
		map_tgt[s.get_storage_type_name()][s.path_pool] = (s.space + s.space_to_add, s.max_space)

	for (a,_,c) in vm.local_disks:	#takes in account only interesting source storages
		if a not in map_src:
			map_src[a] = c
		else:
			map_src[a] += c

	if migrate:
		migration.src_node = src.name
		migration.tgt_node = tgt.name
		migration.vmid = vm.vmid
		migration.vm_t = vm.vm_t
		if vm.running:
			migration.running = 1
		else:
			migration.running = 0


	#iterate over source storages
	for k in map_src.keys():
		tmp = get_storage_from_name(k, src_st)
		tmp2 = None
		space = map_src[k]
		found = False

		#check if images from tmp storage can match one of the target path_pool (/!\ double storages on the same data pool)
		#this time, we map from src storage and not the path_pool attribute because of storage to storage
		for kk in map_tgt[tmp.get_storage_type_name()].keys():
			(s,m) = map_tgt[tmp.get_storage_type_name()][kk]
			if m * (config.limit / 100) > space + s or m < 0: #m < 0 == stockage doesn't exist, so its capacity is not defined
				found = True
				map_tgt[tmp.get_storage_type_name()][kk] = (s+space,m)

				#update node information and Migration object
				if migrate:
					tmp.space_to_add -= space
					tmp2 = get_storage_from_path_pool(kk, tgt_st)
					tmp2.space_to_add += space
					migration.storage_map.append((tmp.get_one_name(), tmp2.get_one_name()))
				break

		#storage not fitable
		if migrate and not found: #new storage
			nr = get_next_nr()
			new_name = '#' + str(nr)
			st_new = Storage()

			st_new.storage_t = tmp.storage_t
			st_new.content = tmp.content
			st_new.path_pool = str(nr)	#it doesn't exist, set anything (unique)
			st_new.node = tgt.name
			st_new.name.add(new_name)
			st_new.max_space = -1	#it means, storage doesn't exit actually
			st_new.space = 0
			st_new.space_to_add = space
			tmp.space_to_add -= space
			tgt.storage_list.append(st_new)
			migration.storage_map.append((tmp.get_one_name(), new_name))

		if not migrate and not found:	#if we count - not creating a Migration object
			count += 1 

	if migrate:	#return the new Migration object
		return migration
	else:	#return the number of fitable storages
		return len(map_src) - count


#src_list: source node list
#tgt_list: target node list
#vm_list: complete dict list (used for target balance calculation and used to modify the position of VMs/CTs)
#from_vm: vm list from src_list
#config: config object - setup by programm arg
def obfd(src_list, tgt_list, vm_list, from_vm, config) -> list:
	if src_list == None or tgt_list == None or vm_list == None or from_vm == None or config == None:
		return None

	migration_order = list()
	nr = 0	#len src_list[0] + src_list[1] + ... max number of iterations for main while

	#preparations
	nodes = list(set(src_list + tgt_list))

	for node in nodes:
		if not node.running:
			return migration_order

	#list of VMs/CTs in sources and targets
	vms = dict()
	for k in nodes:
		vms[k.name] = vm_list[k.name]
		if k.name in config.vm_from:	#if k is in src_list
			nr += len(vm_list[k.name])	#increase nr

	#vm balance
	for k in vms.keys():
		for vm in vms[k]:
			vm.balance_calculation()
	
	#node balance:
	for node in nodes:
		node.balance_calculation(vms[node.name])

	if config.show_info:
		print(sep)
		print('Source nodes:')
		for node in src_list:
			print('\t' + node.name + ': balance = ' + str(node.balance))
		print('Target nodes:')
		for node in tgt_list:
			print('\t' + node.name + ': balance = ' + str(node.balance))


	for i in range(nr):
		if config.balance:	#balance in config
			#we use both sources and targets to get the average balance, but only sources could be selected due to config
			tmp = placement_balance_node(nodes, config)
			if tmp == None or tmp == -1:
				return migration_order
			else:
				to_top(tmp, src_list)
		else:	#select the first node linked to a non-empty vm list
			if not select_next_node_with_vm(src_list, from_vm):
				return migration_order

		vm = from_vm[src_list[0].name].pop(0)	#extract the first VM/CT from the first source node
		src_list[0].nr_vm -= 1
		count = count2 = -1
		target_node = None
		migration = None

		#searching a new host
		for node in tgt_list:
			if not vm.running and vm.vm_t == VmType.QEMU: #vm offline migration
				if node.name not in vm.node_allowed:
					continue
			if (vm.vm_t == VmType.QEMU and node.can_be_host(vm)) or (vm.vm_t == VmType.LXC):
				count2 = count_fitable_storages_or_setup_migration(src_list[0], node, vm, config, False)
				if count < count2:
					target_node = node
					count = count2
				elif count == count2 and node.balance < target_node.balance:
					target_node = node

		#register Migration object
		if type(target_node) == Node and target_node.name != src_list[0].name:

			migration = count_fitable_storages_or_setup_migration(src_list[0], target_node, vm, config, True)
			migration_order.append(migration)
			#update balance
			src_list[0].balance -= vm.balance
			target_node.balance += vm.balance

			#update vm position
			vm.node = target_node.name
			vm_list[src_list[0].name].remove(vm)
			vm_list[target_node.name].append(vm)

	return migration_order


def bfd(src_list, tgt_list, vm_list, from_vm, config, prox):
	if src_list == None or tgt_list == None or vm_list == None or from_vm == None or config == None:
		return None

	migration_order = list()
	last_chance = dict()

	for k in from_vm.keys():
		last_chance[k] = list()

	def one_iter(vms):
		nr = 0

		#prepare nodes and vms
		for node in src_list:
			vms[node.name].sort(key=lambda k: k.cpu_avg, reverse=True)
			nr += len(vms[node.name])
			node.tmp_ram_calculation(vm_list[node.name], config)

		if config.show_info:
			print(sep)
			print('Source nodes:')
			for node in src_list:
				print('\t' + node.name)
			print('Target nodes:')
			for node in tgt_list:
				print('\t' + node.name)

		for i in range(nr):
			src = None
			src_cpu = None
			src_ram = None

			#select src node
			for node in src_list:
				if len(vms[node.name]) == 0: #there are no vm to try to migrate
					continue
				elif src_cpu == None:
					src_cpu = node
				elif node.cpu_avg > src_cpu.cpu_avg:
					src_cpu = node
				if node.tmp_ram > node.max_ram:
					src_ram = node

			#select the node to use as the src node
			if src_cpu != None and src_cpu.cpu_avg > 0.8:
				src = src_cpu
			elif src_ram != None:
				src = src_ram
			#we must empty the node(s)
			elif config.migrate_all:
				src = src_cpu

			if src == None:	#no one is selected, then stop
				return migration_order

			vm = vms[src.name].pop(0)
			tgt = None
			tgt_cpu = 0

			#select the target node
			for node in tgt_list:
				node_cpu = 0
				node_ram = 0
				nr_st = 0

				if node == src:
					node_cpu = node.cpu_avg - vm.cpu_node
					node_ram = node.tmp_ram - vm.max_ram
				else:
					node_cpu = node.cpu_avg
					node_ram = node.tmp_ram
				
				#required:
				if not vm.running and vm.vm_t == VmType.QEMU:
					if node.name not in vm.node_allowed:
						continue
				if node_ram + vm.max_ram > node.max_ram or node_cpu + vm.cpu_node > 1:
					continue

				#tests
				if node_cpu + vm.cpu_node <= 0.8:
					if tgt == None or tgt_cpu > 0.8:
						if config.migrate_all and src == node:
							continue
						tgt = node
						tgt_cpu = node_cpu
						nr_st = count_fitable_storages_or_setup_migration(src, tgt, vm, config, False)
					elif count_fitable_storages_or_setup_migration(src, node, vm, config, False) > nr_st:
						if config.migrate_all and node == src:
							continue
						tgt = node
						tgt_cpu = node_cpu
						nr_st = count_fitable_storages_or_setup_migration(src, tgt, vm, config, False)

				else:
					#if the src can't fit its vm and no target have been selected or the vm need to be migrated to another node
					if tgt == None and (((src.cpu_avg > 1 or src.tmp_ram > src.max_ram) and not config.migrate_all) or (config.migrate_all and node != src)):
						tgt = node
						tgt_cpu = node_cpu
						count_fitable_storages_or_setup_migration(src, tgt, vm, config, False)
					elif tgt != None and node_cpu < tgt_cpu:
						if config.migrate_all and src == node:
							continue
						tgt = node
						tgt_cpu = node_cpu
						nr_st = count_fitable_storages_or_setup_migration(src, tgt, vm, config, False)


			if tgt == None or (tgt != None and config.migrate_all and src == tgt):
				if last_chance != None:
					last_chance[src.name].append(vm)
					continue
				elif last_chance == None and not config.migrate_all:
					continue
				elif last_chance == None and config.migrate_all:
					print('Error: No target node selected - bfd')
					exit(1)
			if not config.migrate_all and tgt == src:
				continue

			#add a new Migration object to the list
			migration = count_fitable_storages_or_setup_migration(src, tgt, vm, config, True)
			upd_lst = list()
			migration_order.append(migration)
			
			vm.node = tgt.name
			vm_list[src.name].remove(vm)
			vm_list[tgt.name].append(vm)

			src.cpu_avg -= vm.cpu_node
			tgt.cpu_avg += vm.cpu_node
			src.tmp_ram_calculation(vm_list[src.name], config)
			tgt.tmp_ram_calculation(vm_list[tgt.name], config)

			upd_list = [src, tgt]
			cpu_on_host_vms(upd_list, vm_list, config, prox)

	#=====================
	one_iter(from_vm)
	if migration_order != list():
		from_vm = last_chance
		last_chance = None
		one_iter(from_vm)

	return migration_order


def algo(src_list, tgt_list, vm_list, from_vm, config, prox = None) -> list:
	if config.algo == AlgoType.OBFD:
		return obfd(src_list, tgt_list, vm_list, from_vm, config)

	elif config.algo == AlgoType.BFD:
		return bfd(src_list, tgt_list, vm_list, from_vm, config, prox)

	else:
		print('ERROR: Unknown algorithm - algo')
		exit(0)


# -------------------------------------------


def to_string(node_list, vm_list) -> int:
	if node_list == None or vm_list == None:
		return -1

	for node in node_list:
		node.to_string()
		for vm in vm_list[node.name]:
			vm.to_string()
		print(sep)
	
	return 0


def fill_node_list(node_list, config, prox) -> int:
	if node_list == None or prox == None:
		return -1

	for node in prox.cluster.resources.get(type='node'):
		n = Node()
		n.fill_base_node(node, config, prox)
		node_list.append(n)


	#getting storages
	for storage in prox.storage.get():
		if 'nodes' in storage.keys():	#name doesn't exist on all nodes
			st_lst = storage['nodes'].split(',')
			for node in node_list:
				if node.name in st_lst and node.running:
					node.fill_node_storage(storage, prox)

		else:	#name exist on all nodes
			for node in node_list:
				if node.running:
					node.fill_node_storage(storage, prox)

	return 0


def fill_vm_list(vm_list, config, prox) -> int:
	if vm_list == None or prox == None:
		return -1

	#init dict with the keys
	offline_node = list()
	for node in prox.cluster.resources.get(type='node'):
		if node['status'] == 'offline':
			offline_node.append(node['node'])
		vm_list[node['node']] = list()

	for vm in prox.cluster.resources.get(type='vm'):
		vm_obj = Vm()
		if vm['node'] in offline_node:
			vm_obj.fill_vm(vm, config, prox, False)
		else:
			vm_obj.fill_vm(vm, config, prox, True)
		vm_list[vm['node']].append(vm_obj)
	
	return 0


def cluster_simulation(vm_list, node_list):
	""""""
	cpu_t = 'cputest'
	gib = (2**30)
	nr = random.randint(2,10)
	nr_cores = random.randint(2, 16)
	ram = random.randint(5,32) * gib
	vm_name = 0

	#nodes
	for i in range(nr):
		node = Node()
		node.name = 'pve-node-' + str(i)
		node.running = True
		node.cpu_t = cpu_t
		node.cpu_avail.add(cpu_t)
		node.cpu = round(random.uniform(0.2, 1.0), 2)
		node.cpu_avg = node.cpu
		node.lavg = ['0.0', '0.0', '0.0']
		node.nr_cores = nr_cores
		node.max_ram = ram
		node_list.append(node)
		st = Storage()
		st.storage_t = StorageType.LVMTHIN
		st.content = 'img'
		st.path_pool = 'data'
		st.node = node.name
		st.name.add('local-lvmthin')
		node.storage_list.append(st)

	#vms
	for node in node_list:
		nr = random.randint(1,10)
		ram = node.max_ram / nr
		total = 0
		vm_list[node.name] = list()

		for i in range(nr):
			vm = Vm()
			vm.name = 'vm-' + str(vm_name)
			vm.vmid = vm_name
			vm_name += 1
			vm.node = node.name
			vm.running = True
			vm.vm_t = VmType.QEMU
			vm.cpu_t = cpu_t
			vm.nr_cores = random.randint(1, nr_cores)
			
			if i == nr-1:
				vm.cpu += round(node.cpu - total, 2)
			else:
				vm.cpu = round(random.uniform(0, node.cpu / nr), 2)
				total += vm.cpu
			vm.cpu_avg = vm.cpu

			vm.ram = int(random.uniform(node.max_ram / (nr * 5), node.max_ram / nr))
			vm.max_ram = vm.ram
			node.ram += vm.ram

			st_name = node.storage_list[0].get_one_name()
			disk_name = vm.name + '-disk-0'
			disk_sz = random.randint(1,5) * gib
			vm.local_disks.append((st_name, disk_name, disk_sz))
			node.storage_list[0].space += disk_sz

			vm_list[node.name].append(vm)

	#storages
	for node in node_list:
		mult = round(random.uniform(1.0,2.0), 2)
		node.storage_list[0].max_space = int(node.storage_list[0].space * mult)


def get_proxmox(config) -> ProxmoxAPI:
	if config == None:
		return None
	return ProxmoxAPI(config.addr, user=config.user, password=config.pwd, port=config.port, verify_ssl=False)


def test_config():
	print('========== TEST ==========')
	node_list = list()
	vm_list = dict()
	migration_order = list()

	config = Config()

	config.fill_config(sys.argv[1:])
	config.addr = '192.168.56.105'
	config.pwd = '123456789'

	if config.command_t == CommandType.HELP:
		help()
		return

	config.check_config_1()

	prox = get_proxmox(config)

	fill_node_list(node_list, config, prox)
	fill_vm_list(vm_list, config, prox)
	cpu_on_host_vms(node_list, vm_list, config, prox)

	config.update_config(node_list)
	config.check_config_2(node_list)


def test_to_string():
	print('========== TEST ==========')
	node_list = list()
	vm_list = dict()
	migration_order = list()
	config = Config()

	config.addr = '192.168.56.105'
	config.pwd = '123456789'

	prox = get_proxmox(config)

	fill_node_list(node_list, config, prox)
	fill_vm_list(vm_list, config, prox)
	cpu_on_host_vms(node_list, vm_list, config, prox)
	to_string(node_list, vm_list)


def test_migrate_qemu():
	print('========== TEST ==========')
	node_list = list()
	vm_list = dict()
	migration_order = list()
	config = Config()

	config.addr = '192.168.56.105'
	config.pwd = '123456789'

	prox = get_proxmox(config)

	fill_node_list(node_list, config, prox)
	fill_vm_list(vm_list, config, prox)

	data = {
		'node' : 'pve-node-3',
		'target' : 'pve-node-1',
		'vmid' : 100,
		'online' : 0,
		'with-local-disks' : 1,
		'targetstorage' : 'local-lvm:local-lvm, testzfs:zfsnode1'
	}

	upid = prox.nodes('pve-node-3').qemu('100').migrate.post(**data)
	blocking = None
	while blocking == None:
		print('in while')
		blocking = Tasks.blocking_status(prox, upid, 10, 1)


def test_migrate_lxc():
	print('========== TEST ==========')
	node_list = list()
	vm_list = dict()
	migration_order = list()
	config = Config()

	config.addr = '192.168.56.105'
	config.pwd = '123456789'

	prox = get_proxmox(config)

	fill_node_list(node_list, config, prox)
	fill_vm_list(vm_list, config, prox)

	data = { #live migration is not yet implemented for LXC CTs
		'node' : 'pve-node-3',
		'target' : 'pve-node-2',
		'vmid' : 103,
		'restart' : 1,
		'target-storage' : 'local-lvm:local-lvm'
	}

	prox.nodes('pve-node-3').lxc('103').migrate.post(**data)


def test_count_and_migration():
	print('========== TEST ==========')
	src = Node()
	tgt = Node()
	vm = Vm()
	config = Config()

	src.name = 'pve-test-1'
	tgt.name = 'pve-test-2'

	#vm
	vm.name = 'toto'
	vm.vmid = 100
	vm.node = src.name
	vm.local_disks.append(('test1-st1','d0',10))

	#storages src
	st1 = Storage()
	st1.storage_t = StorageType.LVMTHIN
	st1.path_pool = 'pool1'
	st1.node = src.name
	st1.name.add('test1-st1')
	st1.max_space = 1000
	st1.space = 10
	src.storage_list.append(st1)

	#storages tgt
	st2 = Storage()
	st2.storage_t = StorageType.LVM
	st2.path_pool = 'pool2'
	st2.node = tgt.name
	st2.name.add('test2-st1')
	st2.max_space = 1000
	st2.space = 10
	tgt.storage_list.append(st2)

	print(count_fitable_storages_or_setup_migration(src, tgt, vm, config, False))
	print(count_fitable_storages_or_setup_migration(src, tgt, vm, config, True))
	print(count_fitable_storages_or_setup_migration(src, tgt, vm, config, False))

	for s in tgt.storage_list:
		s.to_string()


def main(config) -> int:
	if config == None:
		return -1

	node_list = list()
	vm_list = dict()
	src_list = list()
	tgt_list = list()
	from_vm = dict()
	migration_order = list()

	config.check_config_1()

	if config.command_t == CommandType.HELP:
		help()

	elif config.command_t == CommandType.PRINT:
		prox = get_proxmox(config)

		fill_node_list(node_list, config, prox)
		fill_vm_list(vm_list, config, prox)
		cpu_on_host_vms(node_list, vm_list, config, prox)

		to_string(node_list, vm_list)

	elif config.command_t == CommandType.ALGO:
		prox = None

		if not config.simulation:
			prox = get_proxmox(config)

			fill_node_list(node_list, config, prox)
			fill_vm_list(vm_list, config, prox)
			cpu_on_host_vms(node_list, vm_list, config, prox)
			count_vms(node_list, vm_list)

			config.update_config(node_list)
			config.check_config_2(node_list)
		else:
			cluster_simulation(vm_list, node_list)
			cpu_on_host_vms(node_list, vm_list, config, prox)
			count_vms(node_list, vm_list)
			config.update_config(node_list)


		#setup lists
		for k in node_list:
			#vm list from source nodes and source node list
			if k.name in config.vm_from and k.running:
				from_vm[k.name] = list()
				[from_vm[k.name].append(x) for x in vm_list[k.name]]
				src_list.append(k)
			if k.name not in config.ban and k.running:
				tgt_list.append(k)

		if config.show_info:
			print(sep)
			to_string(node_list, vm_list)


		#call algo() wich will root the execution to the right function
		migration_order = algo(src_list, tgt_list, vm_list, from_vm, config)

		if migration_order == []:
			print('INFOS: Nothing to do')
			return 1

		if config.show_info:
			print(sep)
			to_string(node_list, vm_list)
			print(sep)
			i = 1
			for m in migration_order:
				m.to_string(str(i))
				i += 1
			print(sep)
			print(sep)

		save_migrations(tgt_list, migration_order, config)


	elif config.command_t == CommandType.MIGRATE:
		if config.f_json == None:
			print('ERROR: You must select a json file')
			exit(1)

		migrations = None

		try:
			file = open(config.f_json, 'r')
			migrations = json.load(file)
		finally:
			file.close()

		if migrations == None:
			print('ERROR: open() failed')
			exit(1)

		prox = get_proxmox(config)

		do_migrations(migrations, prox)


	else:
		print('ERROR: Unknown command type')
		exit(1)
	
	return 0


def monitor_and_do(config) -> int:
	if config == None:
		return -1

	if config.command_t != CommandType.ALGO:
		print('WARNING: auto is only available for --algo')
		exit(1)
	if config.simulation:
		print('WARNING: Impossible to work on a simulated cluster')
		exit(1)

	sleep_time = 300 #300 secs
	client = None
	ip = list()

	if config.ssh == '':
		print('WARNING: Nothing to monitor')
	
	for e in config.ssh.split(':'):
		e = e.split(',')
		if len(e) != 3:
			print('WARNING: Bad syntax for ssh arg')
			exit(1)
		ip.append(e)

	while True:
		print('INFOS: searching...')
		for e in ip:
			(i,u,p) = e
			client = paramiko.SSHClient()
			client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
			#client.connect(i, username=u, password=p, port=22)
			client.connect(i, username=u, password=p, port=22)

			_,s,_ = client.exec_command('cat /proc/pressure/cpu')
			val = str(s.read()).split(' ')[3].split('=')[1]
			client.close()

			if int(float(val)) >= 0:
				print('INFOS: Algorithm will be run')
				if main(config) == 1:
					break
				print('INFOS: End')

				if config.f_json == None:
					print('INFOS: No json file name in config - skip migration step')

				migrations = None

				try:
					file = open(config.f_json, 'r')
					migrations = json.load(file)
				finally:
					file.close()

				if migrations == None:
					print('ERROR: open() failed')
					exit(1)

				if migrations['required_storages'] != []:
					print('WARNING: You need to create storages. Please, read \'required_storages\' field in ' + config.f_json)
					exit(0)

				config.command_t = CommandType.MIGRATE
				main(config)
				config.command_t = CommandType.ALGO
				break

		print('INFOS: Sleep for ' + str(sleep_time) + ' seconds')
		time.sleep(sleep_time)

	return 0


if __name__ == '__main__':
	config = Config()
	config.fill_config(sys.argv[1:])
	if not config.auto:
		main(config)
	else:
		monitor_and_do(config)
