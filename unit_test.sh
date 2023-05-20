cd src
./execute.elf > ../output_translator.txt
cd ../ProgrammingLanguage/
./execute.sh 2> ../output_native.txt
cd ../

if cmp -s "output_translator.txt" "output_native.txt"
then
   echo "=====! Unit test failed !====="
else
   echo "====== Unit test passed ======"
fi

rm output_translator.txt
rm output_native.txt