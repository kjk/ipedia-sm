rmdir /q /s dist
mkdir dist
copy ARMRel\ipedia_sm_arm_rel.exe dist\ipedia.exe
copy readme_sm.txt dist
copy eula_sm.txt dist
copy ipedia_sm.inf dist
copy ipedia_sm.ini dist
@pushd
cd dist
"C:\Program Files\Windows CE Tools\wce300\Smartphone 2002\tools\CabwizSP.exe" ipedia_sm.inf
move ipedia_sm.CAB ipedia_sm_1_0.cab
@rem ezsetup_sm -l english -i inoah_sm.ini -r readme.txt -e eula.txt -o iNoah_Smartphone_setup.exe
ezsetup_sm -l english -i ipedia_sm.ini -r readme_sm.txt -e eula_sm.txt -o iPedia_Smartphone_setup.exe
@popd