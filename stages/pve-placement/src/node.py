from storage import Storage
from tools import StorageType



def list_match_node_name(name_list, node_list) -> list:
	if name_list == None or node_list == None:
		return None

	names = set()
	ret = list()

	#gettin names
	for e in node_list:
		names.add(e.name)
	
	#matching
	for name in name_list:
		if name not in names:
			ret.append(name)
	return ret


def any_offline_node(name_list, node_list) -> bool | int:
	if name_list == None or node_list == None:
		return -1

	for name in name_list:
		for node in node_list:
			if node.name == name:
				if node.running == False:
					return True
				else:
					break
	return False


def count_online_nodes(node_list) -> int:
	if node_list == None:
		return -1

	count = 0
	for node in node_list:
		if node.running:
			count += 1
	return count


def get_online_nodes(node_list) -> list | int:
	if node_list == None:
		return -1

	ret = list()
	for node in node_list:
		if node.running:
			ret.append(node.name)
	return ret


class Node:
	name = ''				#name of this node
	running = False			#running status
	network_it = None		#list of network interfaces available on this node
	cpu_t = ''				#CPU type
	cpu_avail = None		#list of available CPUs on this node
	cpu = 0					#current CPU load
	cpu_avg = 0				#averrage CPU load of the node
	lavg = None				#load average
	nr_cores = 0			#number of cores
	ram = 0					#actual RAM usage
	max_ram = 0				#max RAM available
	tmp_ram = 0				#used by bfd
	nr_vm = 0				#number of VMs / CTs on this node
	balance = 0				#balance (sum of vm balance)
	storage_list = None		#list of storages available on this node



	def __init__(self):
		self.network_it = set()
		self.cpu_avail = set()
		self.storage_list = list()


	def to_string(self, begin =''):
		print(begin + '--- NODE ---')
		print('\t' + begin + 'name = ' + self.name)
		print('\t' + begin + 'running = ' + str(self.running))
		print('\t' + begin + 'network_interface = ', end='')
		print(self.network_it)
		print('\t' + begin + 'type_cpu = ' + self.cpu_t)
		print('\t' + begin + 'available_cpus = ', end='')
		print(self.cpu_avail)
		print('\t' + begin + 'cpu = ' + str(self.cpu))
		print('\t' + begin + 'estimated_average_cpu_load = ' + str(self.cpu_avg))
		print('\t' + begin + 'load_average = ', end='')
		print(self.lavg)
		print('\t' + begin + 'nr_cores = ' + str(self.nr_cores))
		print('\t' + begin + 'ram = ' + str(self.ram))
		print('\t' + begin + 'estimated_ram_usage_at_most = ' + str(self.tmp_ram))
		print('\t' + begin + 'max_ram = ' + str(self.max_ram))

		for storage in self.storage_list:
			storage.to_string()


	def fill_base_node(self, arg, config, prox) -> int:
		if arg == None or config == None or prox == None:
			return -1

		self.name = arg['node']
		if arg['status'] == 'online':
			self.running = True
		
		if self.running:
			self.cpu = format(arg['cpu'], '.2f')
			self.ram = arg['mem']
			self.max_ram = arg['maxmem']

			self.cpu_average(config, prox)

			#getting CPU model
			tmp = prox.nodes(self.name).status.get()
			self.cpu_t = tmp['cpuinfo']['model']
			self.nr_cores = tmp['cpuinfo']['cores']
			self.lavg = tmp['loadavg']

			#getting available CPUs
			for tmp in prox.nodes(self.name).capabilities.qemu.cpu.get():
				self.cpu_avail.add(tmp['name'])

			#getting network interfaces
			for tmp in prox.nodes(self.name).network.get():
				self.network_it.add(tmp['iface'])
		return 0


	def cpu_average(self, config, prox) -> int:
		if config == None or prox == None:
			return -1

		rrd = None
		cpu_avg = 0
		count = 0

		rrd = prox.nodes(self.name).rrddata.get(timeframe=config.timeframe)

		for e in rrd:
			if 'cpu' in e.keys():
				cpu_avg += float(e['cpu'])
				count += 1

		if count != 0:
			cpu_avg /= count
			self.cpu_avg = float(format(cpu_avg, '.2f'))
		else:
			self.cpu_avg = 0.0
		return 0


	def fill_node_storage(self, arg, prox) -> int:
		if arg == None or prox == None:
			return -1

		st = Storage()

		if arg['type'] == 'dir':
			#check if it already exist
			for tmp in self.storage_list:
				if tmp.storage_t == StorageType.DIR and tmp.path_pool == arg['path']:
					return 0
			if st.fill_dir(arg, self.name, prox) == True:
				self.storage_list.append(st)

		elif arg['type'] == 'lvm':
			#check if it already exist
			for tmp in self.storage_list:
				if tmp.storage_t == StorageType.LVM and tmp.name == arg['storage']:
					return 0
			if st.fill_lvm(arg, self.name, prox) == True:
				self.storage_list.append(st)

		elif arg['type'] == 'lvmthin':
			#check if it already exist
			for tmp in self.storage_list:
				if tmp.storage_t == StorageType.LVMTHIN and tmp.path_pool == arg['thinpool']:
					#add the name of this storage that is linked to that pool
					tmp.name.add(arg['storage'])
					return 0
			if st.fill_lvmthin(arg, self.name, prox) == True:
				self.storage_list.append(st)

		elif arg['type'] == 'zfspool':
			#check if it already exist
			for tmp in self.storage_list:
				if tmp.storage_t == StorageType.ZFS and tmp.path_pool == arg['pool']:
					#add the name of this storage that is linked to that pool
					tmp.name.add(arg['storage'])
					return 0
			if st.fill_zfs(arg, self.name, prox) == True:
				self.storage_list.append(st)

		elif arg['type'] == 'btrfs':
			#check if it already exist
			for tmp in self.storage_list:
				if tmp.storage_t == StorageType.BTRFS and tmp.path_pool == arg['path']:
					return 0
			if st.fill_btrfs(arg, self.name, prox) == True:
				self.storage_list.append(st)

		else:
			print('ERROR: Bad storage type - fill_node_storage')
			exit(1)


	def tmp_ram_calculation(self, vm_list, config):
		gib = 2**30
		tib = 2**40
		tmp = gib * 2
		space = 0

		if self.max_ram < tmp:
			print('WARNING: Node ' + self.name + ' should have at least 2Gib of RAM, otherwise it is considered as a test node')
			exit(1)

		for vm in vm_list:
			tmp += vm.max_ram

		for st in self.storage_list: #each ZFS Tib = +1 Gib RAM
			if st.storage_t == StorageType.ZFS:
				if st.max_space >= 0:
					space += st.max_space
				else:
					#max_space * limit = space + space_to_add
					#max_space = (space + space_to_add) / limit
					space += int((st.space + st.space_to_add) / (config.limit / 100))

		tmp += int(space / tib) * gib
		self.tmp_ram = tmp


	def balance_calculation(self, vm_list) -> int:
		if vm_list == None:
			return -1

		if not self.running:
			print('ERROR: Node ' + self.name + ' is offline - balance_calculation')

		for vm in vm_list:
			self.balance += vm.balance
		
		return 0


	def can_be_host(self, vm) -> bool | int:
		if vm == None:
			return -1

		for vm_it in vm.network_it:
			if vm_it not in self.network_it:
				return False
		
		if vm.cpu_t not in self.cpu_avail:
			return False

		return True
