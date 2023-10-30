# Notes
- proxmox doc: https://pve.proxmox.com/pve-docs/
- Proxmox API REST: https://pve.proxmox.com/pve-docs/api-viewer/index.html
- Wrapper python3 pour l'API REST: https://proxmoxer.github.io/docs/2.0/
- pvesh pour faire des requetes à l'api par ligne de commande


migration des VMs qemu + multiple stockage cibles:
- https://github.com/proxmox/qemu-server/blob/master/PVE/CLI/qm.pm
- https://github.com/proxmox/qemu-server/blob/master/PVE/QemuMigrate.pm
- https://github.com/proxmox/qemu-server/blob/master/PVE/API2/Qemu.pm L.4373
- https://github.com/proxmox/pve-guest-common/blob/master/src/PVE/AbstractMigrate.pm
- https://github.com/proxmox/pve-access-control/blob/master/src/PVE/RPCEnvironment.pm
- https://github.com/proxmox/pve-common/blob/master/src/PVE/RESTEnvironment.pm
- https://github.com/proxmox/pve-guest-common/blob/master/src/PVE/GuestHelpers.pm
- https://github.com/proxmox/pve-common/blob/master/src/PVE/Tools.pm
- https://github.com/proxmox/pve-common/blob/master/src/PVE/JSONSchema.pm
