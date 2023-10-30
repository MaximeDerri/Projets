import json
from proxmoxer import ProxmoxAPI
from proxmoxer.tools import Tasks
from node import Node
from vm import *
from config import *
from storage import *
from tools import VmType


sep = "============================="


class Migration:
    src_node = None         #node name where the vm comes from
    tgt_node = None         #target name for the migration
    vmid = 0                #id
    vm_t = VmType.INIT		#qemu or lxc
    running = 0             #used to restart CT because live migration is no yet implemented for LXC
    storage_map = None      #(src_storage,tgt_storage)[] used for mapping storages



    def __init__(self):
        self.storage_map = list()


    def to_string(self, arg):
        print('--- Migration ' + arg + ' ---')
        print('source_node = ' + self.src_node)
        print('target_node = ' + self.tgt_node)
        print('\tvmid = ' + str(self.vmid))
        print('\ttype = ' + get_vm_type_str(self.vm_t))
        print('storage_map =')
        for st in self.storage_map:
            print('\t', end=' ')
            print(st)


def save_migrations(tgt_list, migration_order, config) -> int:
	if tgt_list == None or migration_order == None or config == None:
		return -1

	#results
	required_storages = list()
	migrations = list()
	to_save = dict()

	for node in tgt_list:
		for st in node.storage_list:
			if st.max_space < 0: #to create
				#max_space * limit = space + space_to_add
				#max_space = (space + space_to_add) / limit
				st.max_space = int((st.space + st.space_to_add) / (config.limit / 100))
				data = {
					'id' : st.get_one_name(),
					'name' : st.path_pool,
					'node' : st.node,
					'storage_type' : st.get_storage_type_name(),
					'needed_size' : st.max_space
				}
				required_storages.append(data)

	for m in migration_order:
		st_lst = list()
		for (s,t) in m.storage_map:
			st_lst.append(s + ':' + t)
		data = {
			'src_node' : m.src_node,
			'tgt_node' : m.tgt_node,
			'vmid' : m.vmid,
			'type' : get_vm_type_str(m.vm_t),
			'running'  : m.running,
			'storage_map' : st_lst
		}
		migrations.append(data)
		
	#combine the two lists in one dict
	to_save['required_storages'] = required_storages
	to_save['migrations'] = migrations
	if config.f_json == None:
		print(to_save)
	else:
		try:
			file = open(config.f_json, 'w')
			json.dump(to_save, file)
		finally:
			file.close()

	return 0


def do_migrations(migrations, prox) -> int:
	if migrations == None or prox == None:
		return -1

	#checking storages
	required_storages = migrations['required_storages']
	storages = prox.cluster.resources.get(type='storage')

	for req_st in required_storages:
		found = False
		for st in storages:
			if req_st['name'] == st['storage']: #found
				#checking node:
				if req_st['node'] != st['node']:
					continue
				#checking storage type
				if ((req_st['storage_type'] == 'zfs' or req_st['storage_type'] == 'zfspool') and st['plugintype'] != 'zfspool') \
				or (req_st['storage_type'] != st['plugintype']):
					print('ERROR: Storage ' + req_st['name'] + ' from node ' + req_st['node'] + ' is of type ' + st['plugintype'] +  ' but ' + req_st['storage_type'] + ' was required')
					exit(1)
				
				#checking size:
				if st['maxdisk'] - st['disk'] < req_st['needed_size']:
					print('ERROR: Storage ' + req_st['name'] + ' from node ' + req_st['node'] + ' must have ' + str(req_st['needed_size']) + ' bytes but the actual size is smaller')
					exit(1)
				found = True
				break
		if not found:
			print('ERROR: storage ' + req_st['name'] + ' not found')
			exit(1)


	#migrations
	def get_name_aux(storages, id):
		for st in storages:
			if st['id'] == id:
				return st['name']
		return None

	for m in migrations['migrations']:
		data = None
		upid = None
		#target storage preparation
		targetstorage = ''
		for e in m['storage_map']:
			tmp = e.split(':')
			if tmp[1][0] == '#': #search the target storage name in required_storages list
				nm = get_name_aux(required_storages, tmp[0])
				if nm == None:
					print('ERROR: Unknown required storage id - main')
					exit(1)
				#update targetstorage
				if targetstorage != '':
					targetstorage += ','
				targetstorage += tmp[0] + ':' + nm
			else:
				if targetstorage != '':
					targetstorage += ','
				targetstorage += e
				
		if m['type'] == 'qemu':
			data = {
				'node' : m['src_node'],
				'target' : m['tgt_node'],
				'vmid' : m['vmid'],
				'online' : m['running'],
				'with-local-disks' : 1,
				'targetstorage' : targetstorage
			}
			#start
			upid = prox.nodes(m['src_node']).qemu(m['vmid']).migrate.post(**data)

		elif m['type'] == 'lxc':
			data = {
				'node' : m['src_node'],
				'target' : m['tgt_node'],
				'vmid' : m['vmid'],
				'restart' : m['running'],
				'target-storage' : targetstorage
			}
			#start
			upid = prox.nodes(m['src_node']).lxc(m['vmid']).migrate.post(**data)
			
		else:
			print("ERROR: Unknown vm type - main")
			exit(1)
		print(sep)
		print('Start migration: ' + str(data))

		blocking = None
		while blocking == None:
			blocking = Tasks.blocking_status(prox, upid, polling_interval=2.0)
		print('End')
	
	return 0
