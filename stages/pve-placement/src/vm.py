from tools import VmType



#extract the bridge name
def extract_bridge(arg) -> str | int:
	if arg == None:
		return -1

	tab = arg.split(',')
	for e in tab:
		if e[:6] == 'bridge':
			return e[7:]
	
	#not found
	return -1


def get_vm_type_enum(arg) -> VmType:
	if arg == 'qemu':
		return VmType.QEMU
	elif arg == 'lxc':
		return VmType.LXC
	else:
		print('ERROR: Unknown vm type - get_vm_type_enum')
		exit(1)


def get_vm_type_str(arg) -> str:
	if arg == VmType.QEMU:
		return 'qemu'
	elif arg == VmType.LXC:
		return'lxc'
	else:
		print('ERROR: Unknown vm type - get_vm_type_str')
		exit(1)


class Vm:
	name = ''				#name of this vm
	vmid = 0				#id of this vm
	node = ''				#name of the node that contain this vm
	running = False			#running status
	vm_t = VmType.INIT		#VM / Container
	cpu_t = ''				#CPU type
	nr_cores = 0			#number of cores
	cpu = 0					#current CPU load
	cpu_avg = 0				#average CPU load
	cpu_node = 0			#estimated CPU load on the node
	cpu_host = 0			#estimated CPU load on host
	ram = 0					#current RAM usage
	max_ram = 0				#max RAM available
	balance = 0				#balance
	network_it = None		#network interfaces used by this vm
	node_allowed = None		#list of node names for offline migration
	local_disks = None		#(storage_name, disk_name, total_space) / disks of this vm



	def __init__(self):
		self.local_disks = list()
		self.network_it = set()
		self.node_allowed = set()


	def to_string(self, begin=''):
		print(begin + '--- VM/CT ---')

		tmp = get_vm_type_str(self.vm_t)

		print('\t' + begin + 'name = ' + self.name)
		print('\t' + begin + 'vmid = ' + str(self.vmid))
		print('\t' + begin + 'node = ' + self.node)
		print('\t' + begin + 'type = ' + tmp)
		print('\t' + begin + 'running = ' + str(self.running))
		print('\t' + begin + 'type_cpu = ' + self.cpu_t)
		print('\t' + begin + 'cores = ' + str(self.nr_cores))
		print('\t' + begin + 'cpu = ' + str(self.cpu))
		print('\t' + begin + 'average_cpu = ' + str(self.cpu_avg))
		print('\t' + begin + "estimated_cpu_load_on_the_node = " + str(self.cpu_node))
		print('\t' + begin + "estimated_cpu_load_on_the_host = " + str(self.cpu_host))
		print('\t' + begin + 'ram = ' + str(self.ram))
		print('\t' + begin + 'max_ram = ' + str(self.max_ram))
		print('\t' + begin + 'network_interfaces = ', end='')
		print(self.network_it)
		
		if not self.running:	#node_allowed is only for offline migration
			print('\t' + begin + 'node_allowed = ', end='')
			print(self.node_allowed)
		
		print('\t' + begin + 'local_disks =')
		for disk in self.local_disks:
			print('\t\t' + begin, end='')
			print(disk)


	def fill_vm(self, arg, config, prox, node_status) -> int:
		if arg == None or config == None or prox == None:
			return -1

		if not node_status: #node offline
			self.vmid = arg['vmid']
			self.node = arg['node']
			self.vm_t = get_vm_type_enum(arg['type'])
			return

		self.name  = arg['name']
		self.vmid = arg['vmid']
		self.node = arg['node']
		
		if arg['status'] == 'stopped':
			self.running = False
		else:
			self.running = True

		self.vm_t = get_vm_type_enum(arg['type'])

		self.cpu = arg['cpu']
		self.ram = arg['mem']
		self.max_ram = arg['maxmem']
		self.balance = 0

		tmp = None
		if self.vm_t == VmType.QEMU:
			tmp = prox.nodes(self.node).qemu(str(self.vmid)).config.get()
			self.cpu_t = tmp['cpu']
		else:	#LXC
			tmp = prox.nodes(self.node).lxc(str(self.vmid)).config.get()
			#lxc run on the cpu host - see later if we check the node cpu type # TODO TOREMOVE

		self.nr_cores = tmp['cores']

		self.cpu_average(config, prox)

		#searching for netN keys -  network interfaces
		for e in tmp.keys():
			if len(e) >= 4:
				if e[:3] == 'net':
					#searching for bridge part
					self.network_it.add(extract_bridge(tmp[e]))
		
		#migration informations
		#QEMU
		if self.vm_t == VmType.QEMU:
			tmp = prox.nodes(self.node).qemu(str(self.vmid)).migrate.get()
			k = tmp.keys()
			if 'allowed_nodes' in k: #present if vm is offline
				[self.node_allowed.add(x) for x in tmp['allowed_nodes']]

				#check if storage capacity is the cause of not allowed nodes field
				for kk in tmp['not_allowed_nodes'].keys():
					if len(tmp['not_allowed_nodes'][kk]) == 1 and next(iter(tmp['not_allowed_nodes'][kk])) == 'unavailable_storages':
						self.node_allowed.add(kk)

			if 'local_disks' in k:
				for disk in tmp['local_disks']:
					if disk['cdrom']:
						print("WARNING: CDROM detected. VM type: QEMU, id: " + str(self.vmid))
						exit(0)
					else:
						st, nm = disk['volid'].split(':')
						sz = disk['size']
						self.local_disks.append((st, nm, sz))
		
		#LXC
		#it's different than QEMU, pre-condition doesn't exist (like Qemu with GET migration)
		else:
			#we have to use such a function because lxc doesn't provide a pre-migration command like qemu
			def parse_aux(val):
				n = '0'
				test = False
				for c in val:
					if ord(c) >= ord('0') and ord(c) <= ord('9'):
						n += c
					else:
						test = True
						break
				if test:
					return int((2**30) * int(n))	#the value is a multiple of 1 GiB, so convert it to bytes (ex: in json, '1G')
				else:
					return int(n) #if the value is not a multiple, it is already in bytes
			
			#rootfs
			tmp_lst = tmp['rootfs'].split(':')
			st = tmp_lst[0]
			tmp_lst = tmp_lst[1].split(',')
			nm = tmp_lst[0]
			tmp_lst = tmp_lst[1].split('=')
			sz = parse_aux(tmp_lst[1])
			self.local_disks.append((st, nm, sz))

			#searching for other local disks
			for k in tmp.keys():
				if k[:2] == 'mp':
					tmp_lst = tmp[k].split(':')
					st = tmp_lst[0]
					tmp_lst = tmp_lst[1].split(',')
					nm = tmp_lst[0]
					tmp_lst = tmp_lst[2].split('=') #skip 'mp=...'
					sz = parse_aux(tmp_lst[1])
					self.local_disks.append((st, nm, sz))
		
		return 0


	def cpu_average(self, config, prox) -> int:
		if config == None or prox == None:
			return -1

		rrd = None
		cpu_avg = 0
		count = 0

		if self.vm_t == VmType.QEMU:
			rrd = prox.nodes(self.node).qemu(self.vmid).rrddata.get(timeframe=config.timeframe)
		elif self.vm_t == VmType.LXC:
			rrd = prox.nodes(self.node).lxc(self.vmid).rrddata.get(timeframe=config.timeframe)
		else:
			print('ERROR: Unknown type - cpu_on_host_calculation')
			exit(1)
		
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


	def cpu_on_host(self, vms_A_L_sum, node, config) -> int:
		if node == None or config == None:
			return -1

		lavg = 0

		if config.lavg == 1:
			lavg = float(node.lavg[0])
		elif config.lavg == 5:
			lavg = float(node.lavg[1])
		elif config.lavg == 15:
			lavg = float(node.lavg[2])
		else:
			print('ERROR: Unknown load average period')
		
		lavg /= node.nr_cores
		self.cpu_host = float(format((lavg * self.cpu_avg * self.nr_cores) / vms_A_L_sum, '.2f'))
		self.cpu_node = float(format((node.cpu_avg * self.cpu_avg * self.nr_cores) / vms_A_L_sum, '.2f'))

		return 0


	def balance_calculation(self):
		if self.running:
			self.balance = ((self.ram / self.max_ram) * 100 + (self.cpu * 100 * self.nr_cores)) / 2
		else:	#we supposed that the vm use 50% of it's capacity
			self.balance = (50 + (50 * self.nr_cores)) / 2


def cpu_on_host_vms(node_list, vm_list, config, prox) -> int:
	#Values:
	# Ai = cores allocated to VM i
	# Li = load of VM i
	# CPU = load average of the node / number of cores of the node
	#
	#calc:
	# CPU * ((Ax * Lx) / sum(i=0 to l, Ai * Li))

	if node_list == None or vm_list == None or config == None:
		return -1
	if not config.simulation and prox == None:
		return -1

	for node in node_list:
		vms_A_L_sum = 0
		if node.running:
			for vm in vm_list[node.name]:
				if vm.running:
					vms_A_L_sum += (vm.cpu_avg * vm.nr_cores)

			for vm in vm_list[node.name]:
				if vm.running:
					vm.cpu_on_host(vms_A_L_sum, node, config)

	return 0
