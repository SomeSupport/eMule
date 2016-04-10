
ID3libCOMps.dll: dlldata.obj ID3libCOM_p.obj ID3libCOM_i.obj
	link /dll /out:ID3libCOMps.dll /def:ID3libCOMps.def /entry:DllMain dlldata.obj ID3libCOM_p.obj ID3libCOM_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del ID3libCOMps.dll
	@del ID3libCOMps.lib
	@del ID3libCOMps.exp
	@del dlldata.obj
	@del ID3libCOM_p.obj
	@del ID3libCOM_i.obj
