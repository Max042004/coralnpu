#!/bin/bash
# Show what is using the VM's root filesystem (read-only report).
df -h / | tail -1
echo "--- /tmp entries (size, owner, name)"
sudo du -xsh /tmp/* /tmp/.[!.]* 2>/dev/null | sort -h | tail -12
ls -ld /tmp/tmp.* 2>/dev/null | head -20
