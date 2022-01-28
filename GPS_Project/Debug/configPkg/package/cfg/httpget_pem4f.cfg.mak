# invoke SourceDir generated makefile for httpget.pem4f
httpget.pem4f: .libraries,httpget.pem4f
.libraries,httpget.pem4f: package/cfg/httpget_pem4f.xdl
	$(MAKE) -f C:\Users\kerem\workspace_v10\GPS_Project/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\kerem\workspace_v10\GPS_Project/src/makefile.libs clean

