rmdir /q /s dist_ppc
mkdir dist_ppc
copy ARMRel\ipedia_ppc_arm_rel.exe dist_ppc\ipedia.exe
copy readme_ppc.txt dist_ppc
copy eula_sm.txt dist_ppc
copy ipedia_ppc.inf dist_ppc
copy ipedia_ppc.ini dist_ppc
cd dist_ppc
"C:\Program Files\Windows CE Tools\wce300\Pocket PC 2002\support\ActiveSync\windows ce application installation\cabwiz\cabwiz.exe" ipedia_ppc.inf
move ipedia_ppc.CAB ipedia_ppc_1_0.cab
ezsetup -l english -i ipedia_ppc.ini -r readme_ppc.txt -e eula_sm.txt -o ipedia_ppc_1_0_setup.exe
del *.txt *.ini *.inf *.DAT ipedia.exe