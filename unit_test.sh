cd src
./main 2> ../output_translator.txt
cd ../ProgrammingLanguage/
./execute.sh 2> ../output_native.txt
cd ../

if cmp -s "output_translator.txt" "output_native.txt"
then
   echo "====== Unit test passed ======"
else
   echo "=====! Unit test failed !====="
fi