from tools import CommandType, AlgoType, ArgType
from node import *



def help():
	s = 'Commands:\n'
	
	s += '\t--help: print this help\n'
	
	s += '\t--algo <...>: run an algorithm\n'
	s += '\t\taddr=X	-> cluster address\n'
	s += '\t\tport=X	-> cluster port - 8006 by default\n'
	s += '\t\tuser=X	-> username - root@pam by default\n'
	s += '\t\tpwd=X	-> password\n'
	s += '\t\talgo=[obfd,bfd]	-> the algo to use\n'
	s += '\t\tshow=[y,n]	-> show some information before and after the algorithm - default is enable\n'
	s += '\t\tban=n1,...	-> ban online target nodes - default is none\n'
	s += '\t\tfrom=n1,...	-> online nodes to use as source - default is all\n'
	s += '\t\tlimit=[1;100]	-> limit the filling of storages to X% - default is 80%\n'
	s += '\t\tjson=file	-> name of the json file to save results\n'
	s += '\t\tsimulation=[y,n]	-> start the algorithm on a random simulated cluster - default is disable\n'
	s += '\t\tauto=[y,n]	-> read periodically /proc/pressure/cpu and launch algo / migration if it\'s necessary - default is disable\n'
	s += '\t\tssh=ip1,user1,pwd1:...	-> IPs, USERs and PWDs informations to monitor the nodes if auto=y	'
	s += '\t\t===obfd===\n'
	s += '\t\t\tbalance=[y,n]	-> enable balance usage. Stop the algorithm if the balance is respected - default is y(es)\n'
	s += '\t\t\tscale=[0,100]	-> scale X% is used to make decisions about stopping the algorighm if balance is enable - default is 10%\n'
	s += '\t\t===bfd===\n'
	s += '\t\t\tlavg=[1,5,15]	-> load average period to use for the algorithm. 1 or 5 or 15 minutes - default is 15\n'
	s += '\t\t\ttimeframe=[hour,day,week,month,year]	-> timeframe to produce an average CPU load - default is hour\n'
	s += '\t\t\tmigrate_all=[y,n]	-> try to migrate every VMs and CTs selected in from list - default is disable\n'

	s += '\t--migrate <...>: launch migrations\n'
	s += '\t\taddr=X	-> cluster address\n'
	s += '\t\tport=X	-> cluster port - 8006 by default\n'
	s += '\t\tuser=X	-> username - root@pam by default\n'
	s += '\t\tpwd=X	-> password\n'

	s += '\t--print <...>: print cluster informations\n'
	s += '\t\taddr=X	-> cluster address\n'
	s += '\t\tport=X	-> cluster port - 8006 by default\n'
	s += '\t\tuser=X	-> username - root@pam by default\n'
	s += '\t\tpwd=X	-> password\n'
	s += '\t\tjson=f	-> name of the json file to use\n'

	print(s)


class Config:
	addr = ''						#cluster address
	port = 8006						#cluster port
	user = 'root@pam'				#username for loggin
	pwd = ''						#password for loggin
	ssh = ''						#ips,user and pwds for ssh
	ban = None						#nodes where VMs / CTs can't be placed in
	vm_from = None					#nodes where VMs / CTs  should be selected for migration
	limit = 80						#percentage. limit the filling of node storages
	balance = True					#enable the "balance" - used to decide if the algos must stop before the end
	migrate_all = False				#used to try to empty a node / nodes
	scale = 10						#if balance is enable, algos will be able to check if nodes are balanced (average of every node balance with +- balance_r %)
	lavg = 15						#load average period
	timeframe = 'hour'				#timeframe for rrddata
	f_json = None					#json file to save results
	show_info = True				#show the configuration before the algorithm, after and the migration list
	algo = AlgoType.BFD				#the algo to use
	simulation = False				#start the algorithm on a random simulated cluster
	auto = False					#read periodically /proc/pressure/cpu and launch algo / migration
	command_t = CommandType.INIT	#command type
	#see later for migration: question and then run or write to a file ?



	def __init__(self):
		self.ban= list()
		self.vm_from = list()


	def fill_config(self, arg) -> int:
		if arg == None:
			return -1

		for e in arg:
			#command type
			if e == ArgType.HELP:
				self.command_t = CommandType.HELP
				return 0
			elif e == ArgType.ALGO:
				if self.command_t != CommandType.INIT:
					self.command_t = CommandType.HELP
				else:
					self.command_t = CommandType.ALGO
					continue
			elif e == ArgType.MIGRATE:
				if self.command_t != CommandType.INIT:
					self.command_t = CommandType.HELP
				else:
					self.command_t = CommandType.MIGRATE
					continue
			elif e == ArgType.PRINT:
				if self.command_t != CommandType.INIT:
					self.command_t = CommandType.HELP
				else:
					self.command_t = CommandType.PRINT
					continue
			
			#args
			a = e.split('=')
			if len(a) != 2:
				self.command_t = CommandType.HELP
				return 0

			if a[0] == ArgType.ADDR:
				self.addr = a[1]

			elif a[0] == ArgType.PORT:
				if not a[1].isdigit():
					print('WARNING: ' + a[0] + ' is not a digit string')
					continue
				self.port = int(a[1])

			elif a[0] == ArgType.USER:
				self.user = a[1]

			elif a[0] == ArgType.PWD:
				self.pwd = a[1]
			
			elif a[0] == ArgType.SSH:
				self.ssh = a[1]

			elif a[0] == ArgType.BAN:
				self.ban = list(set(a[1].split(','))) #unique values

			elif a[0] == ArgType.FROM:
				self.vm_from = list(set(a[1].split(','))) #unique values
				if len(self.vm_from) == 1 and self.vm_from[0] == 'all':
					self.vm_from = list() # all == []

			elif a[0] == ArgType.LIMIT:
				if not a[1].isdigit():
					print('WARNING: ' + a[0] + ' is not a digit string')
					continue
				self.limit = int(a[1])

			elif a[0] == ArgType.BALANCE:
				if a[1] == 'y':
					self.balance = True
				else:
					self.balance = False

			elif a[0] == ArgType.MIGRATE_ALL:
				if a[1] == 'y':
					self.migrate_all = True
				else:
					self.migrate_all = False

			elif a[0] == ArgType.SCALE:
				if not a[1].isdigit():
					print('WARNING: ' + a[0] + 'is not a digit string')
					continue
				self.scale = int(a[1])

			elif a[0] == ArgType.LAVG:
				if not a[1].isdigit():
					print('WARNING: ' + a[0] + 'is not a digit string')
					continue
				self.lavg = int(a[1])

			elif a[0] == ArgType.TIMEFRAME:
				self.timeframe = a[1]

			elif a[0] == ArgType.JSON:
				self.f_json = a[1]

			elif a[0] == ArgType.SHOW:
				if a[1] == 'y':
					self.show_info = True
				else:
					self.show_info = False
			
			elif a[0] == ArgType.SIMULTION:
				if a[1] == 'y':
					self.simulation = True
				else:
					self.simulation = False
			
			elif a[0] == ArgType.AUTO:
				if a[1] == 'y':
					self.auto = True
				else:
					self.auto = False

			elif a[0] == ArgType.ALG:
				if a[1] == AlgoType.OBFD:
					self.algo = AlgoType.OBFD
				elif a[1] == AlgoType.BFD:
					self.algo = AlgoType.BFD

			#unknown arg
			else:
				self.command_t = CommandType.HELP
				return 0

		if self.command_t == CommandType.INIT:
			self.command_t = CommandType.HELP
		
		return 0


	def get_real_ban_list(self, node_list) -> int:
		if node_list == None:
			return -1

		if len(self.ban) == 1 and self.ban[0] == 'all':
			self.ban = get_online_nodes(node_list)
		return 0


	def get_real_from_list(self, node_list) -> int:
		if node_list == None:
			return -1

		if len(self.vm_from) == 1 and self.vm_from[0] == 'all' \
		or self.vm_from == list():
			self.vm_from = get_online_nodes(node_list)
		return 0


	#update from and real list (all -> list of node names)
	def update_config(self, node_list) -> int:
		if node_list == None:
			return -1
		self.get_real_ban_list(node_list)
		self.get_real_from_list(node_list)
		return 0


	def check_config_1(self):
		if self.command_t != CommandType.HELP and self.command_t != CommandType.ALGO \
		and self.command_t != CommandType.MIGRATE and self.command_t != CommandType.PRINT:
			print('ERROR: Unknown command type')
			exit(1)

		if self.command_t == CommandType.ALGO:
			if self.limit < 1 or self.limit > 100:
				print('ERROR: limit need to be in [0,100]')
				exit(1)
			if self.scale < 0 or self.scale > 100:
				print('ERROR: scale need to be in [0,100]')
				exit(1)
			if self.algo != AlgoType.BFD and self.algo != AlgoType.OBFD:
				print('ERROR: Unknown algorithm')
				exit(1)

		if self.command_t == CommandType.ALGO or self.command_t == CommandType.PRINT:
			if self.lavg != 1 and self.lavg != 5 and self.lavg != 15:
				print('ERROR: lavg should be 1 or 5 or 15')
				exit(1)
			if self.timeframe != 'hour' and self.timeframe != 'day' and self.timeframe != 'week' \
			and self.timeframe != 'month' and self.timeframe != 'year':
				print('ERROR: timeframe should be hour or day or week or month or year')


	def check_config_2(self, node_list) -> int:
		if node_list == None:
			return -1

		if self.command_t == CommandType.HELP:
			return 0

		elif self.command_t == CommandType.ALGO:
			#check if node names exist
			#ban list
			tmp = list_match_node_name(self.ban, node_list)
			if tmp != []:
				print('ERROR: ban list is incorrect. Node names ', end='')
				print(tmp, end='')
				print(" don't exist")
				exit(1)
			#any node offline ?
			if any_offline_node(self.ban, node_list):
				print("ERROR: Offline node detected on ban list")
				exit(1)
			#every nodes in the list are online. let's check if all online nodes have been selected
			if (len(self.ban) == 1 and self.ban[0] == 'all') \
			or len(self.ban) == count_online_nodes(node_list):
				print('ERROR: all nodes cannot be banned')
				exit(1)

			#vm_from list
			if self.vm_from == [] or len(self.vm_from) == 1 and self.vm_from[0] == 'all':
				return 0
			tmp = list_match_node_name(self.vm_from, node_list)
			if tmp != []:
				print('ERROR: from list is incorrect. Node names ', end='')
				print(tmp, end='')
				print(" don't exist")
				exit(1)
			#any node offline ?
			if any_offline_node(self.vm_from, node_list):
				print("ERROR: Offline node detected on from list")
				exit(1)
			return 0

		elif self.command_t == CommandType.MIGRATE:
			return 0
		
		elif self.command_t == CommandType.PRINT:
			return 0

		else:
			print('ERROR: Unknown command type')
			exit(1)
