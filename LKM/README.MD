# Some samples of trivial LKM rootkits 
## Auditd

sudo nano /etc/audit/rules.d/30-kernel-rootkit.rules

sudo augenrules --load

sudo auditctl -l | grep -E 'ldpreload|lib-modification|etc-modification|tmp-execution'

sudo auditctl -s

sudo ausearch -k ldpreload -i

sudo ausearch -k lib-modification -i

sudo ausearch -k usr-lib-modification -i

sudo ausearch -k lib64-modification -i

sudo ausearch -k usr-lib64-modification -i

sudo ausearch -k etc-modification -i

sudo ausearch -k tmp-execution -i

sudo ausearch -k vartmp-execution -i
