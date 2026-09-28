# Some samples of trivial LKM rootkits 
## Auditd

sudo nano /etc/audit/rules.d/lkm-rootkits.rules

sudo augenrules --load

sudo auditctl -l | grep -E 'kernel_module|kernel_kexec|kernel_module_tool|kernel_module_file|kernel_module_config|kernel_sysctl|kernel_module_config'

sudo auditctl -s

sudo ausearch -k kernel_module -i


