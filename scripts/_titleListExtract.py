#generate title comparison list from downgrade packs
import os,sys
import glob

list=glob.glob("*.cia")
offset=0x2f9c
index=0
titleID=""
version=""
entry=""

titlelist=open("_titleList.py","w")
titlelist.write("title_lists = {\n\t'n3ds_9.2.0-20J.bin': (\n\t")

for i in list:
	if(index % 5 == 0 and index != 0):
		titlelist.write("\n\t")
	f=open(i,"rb")
	f.seek(offset)
	version=f.read(2).encode('hex')
	f.seek(offset-0x50)
	titleID=f.read(8).encode('hex')
	titlelist.write("(0x%s, 0x%s), " % (titleID,version) )
	f.close()
	print titleID,version
	index+=1

titlelist.write("\n\t),\n}")
titlelist.close()

print "\n%d titles dumped to _titleList.py" % index