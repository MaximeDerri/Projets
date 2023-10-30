from enum import IntEnum, Enum



class StorageType(IntEnum):
	INIT = 0
	LVM = 1
	LVMTHIN = 2
	ZFS = 3
	DIR = 4
	BTRFS = 5


class VmType(IntEnum):
	INIT = 0
	QEMU = 1
	LXC = 2
	NODE = 3
	STORAGE = 4


class CommandType(IntEnum):
	INIT = 0
	ALGO = 1
	MIGRATE = 2
	HELP = 3
	PRINT = 4
	ERR = 5


class AlgoType(str, Enum):
	INIT = ""
	BFD = "bfd"
	OBFD = "obfd"

class ArgType(str, Enum):
	HELP = '--help'
	ALGO = '--algo'
	MIGRATE = '--migrate'
	PRINT = '--print'

	ADDR = 'addr'
	PORT = 'port'
	USER = 'user'
	PWD = 'pwd'
	BAN = 'ban'
	FROM = 'from'
	LIMIT = 'limit'
	BALANCE = 'balance'
	MIGRATE_ALL = 'migrate_all'
	SCALE = 'scale'
	JSON = 'json'
	SHOW = 'show'
	ALG = 'algo'
	LAVG = 'lavg'
	TIMEFRAME = 'timeframe'
	SIMULTION = "simulation"
	AUTO = 'auto'
	SSH = 'ssh'
