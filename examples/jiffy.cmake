# Kickstart for jiffy, the punchboot board in src/board/jiffy
#
#   cmake -C examples/jiffy.cmake -DKICKSTART_KEYSTORE=keystore.bpak ...
#
# The devices follow default_gpt_tbl in punchboot's src/board/jiffy/board.c,
# the eMMC user area is mmcblk0:
#
#   p1 System A          boot.bpak, loaded by punchboot
#   p2 System B
#   p3 Root A            root image, a bpak with a squashfs and its merkle tree
#   p4 Root B
#   p5 PB State Primary
#   p6 PB State Backup

set(KICKSTART_ROOT_A /dev/mmcblk0p3 CACHE STRING "")
set(KICKSTART_ROOT_B /dev/mmcblk0p4 CACHE STRING "")
set(KICKSTART_PBSTATE_PRIMARY /dev/mmcblk0p5 CACHE STRING "")
set(KICKSTART_PBSTATE_BACKUP /dev/mmcblk0p6 CACHE STRING "")
