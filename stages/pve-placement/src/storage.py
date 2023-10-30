from tools import StorageType



def get_next_nr() -> int:
	get_next_nr.nr_next += 1
	return get_next_nr.nr_next
get_next_nr.nr_next = -1


class Storage:
	storage_t = StorageType.INIT	#lvm / lvmthin / zfs / dir
	content = ''					#images,rootdir; vztmpl,iso,backup; ...
	path_pool = ''					#name of the path / memory pool
	node = ''						#name of the node that contain this storage space
	name = None						#list of storages that use path_pool (we can have multiple storages linked to the same pool)
	max_space = 0					#storage capacity? -1 = storage doesn't exist actually
	space = 0						#used space
	space_to_add = 0				#space that could be add to this storage space (filled by the algos)



	def __init__(self):
		self.name = set()

	
	def get_storage_type_name(self) -> str:
		if self.storage_t == StorageType.DIR:
			return 'dir'
		elif self.storage_t == StorageType.LVM:
			return 'lvm'
		elif self.storage_t == StorageType.LVMTHIN:
			return 'lvmthin'
		elif self.storage_t == StorageType.ZFS:
			return 'zfs'
		elif self.storage_t == StorageType.BTRFS:
			return 'btrfs'
		else:
			print('ERROR: unknown storage type - to_string')
			exit(1)


	def to_string(self, begin=''):
		print(begin + '--- STORAGE ---')

		tmp = self.get_storage_type_name()

		print('\t' + begin + 'type = ' + tmp)
		print('\t' + begin + 'content = ' + self.content)
		print('\t' + begin + 'path_pool = ' + self.path_pool)
		print('\t' + begin + 'node = ' + self.node)
		print('\t' + begin + 'name = ', end='')
		print(self.name)
		print('\t' + begin + 'max_space = ' + str(self.max_space))
		print('\t' + begin + 'space = ' + str(self.space))
		print('\t' + begin + 'space_to_add = ' + str(self.space_to_add))


	#these functions return False if the storage is not active
	def fill_storage_base(self, arg, node_name, prox) -> bool | int:
		if arg == None or node_name == None or prox == None:
			return -1

		self.name.add(arg['storage'])
		tmp = prox.nodes(node_name).storage(arg['storage']).status.get()
		if not tmp['active']:
			return False

		self.content = arg['content']
		self.node = node_name
		self.max_space = tmp['total']
		self.space = tmp['used']
		return True

	def fill_dir(self, arg, node_name, prox) -> bool | int:
		if arg == None or node_name == None or prox == None:
			return -1

		if arg == None or node_name == None:
			return -1

		if not self.fill_storage_base(arg, node_name, prox):
			return False
		self.storage_t = StorageType.DIR
		self.path_pool = arg['path']
		return True

	def fill_lvm(self, arg, node_name, prox) -> bool | int:
		if arg == None or node_name == None or prox == None:
			return -1

		if not self.fill_storage_base(arg, node_name, prox):
			return False
		self.storage_t = StorageType.LVM
		self.path_pool = ""
		return True

	def fill_lvmthin(self, arg, node_name, prox) -> bool | int:
		if arg == None or node_name == None or prox == None:
			return -1

		if not self.fill_storage_base(arg, node_name, prox):
			return False
		self.storage_t = StorageType.LVMTHIN
		self.path_pool = arg['thinpool']
		return True

	def fill_zfs(self, arg, node_name, prox) -> bool | int:
		if arg == None or node_name == None or prox == None:
			return -1

		if not self.fill_storage_base(arg, node_name, prox):
			return False
		self.storage_t = StorageType.ZFS
		self.path_pool = arg['pool']
		return True

	def fill_btrfs(self, arg, node_name, prox) -> bool | int:
		if arg == None or node_name == None or prox == None:
			return -1

		if not self.fill_storage_base(arg, node_name, prox):
			return False
		self.storage_t = StorageType.BTRFS
		self.path_pool = arg['path']
		return True
	
	def get_one_name(self) -> str:
		for n in self.name:
			return n #return just the first name of the set


def get_storage_from_name(name, st_list) -> Storage | int:
	if st_list == None:
		return -1
	for s in st_list:
		if name in s.name:
			return s
	
	#not found
	return -1


def get_storage_from_path_pool(pp, st_list) -> Storage | int:
	if st_list == None:
		return -1
	for s in st_list:
		if pp == s.path_pool:
			return s
	
	#not found
	return -1
