set CHUCK_DIR=C:/Program Files/ChucK

rem Download libsndfile
if not exist "thirdparty/libsndfile-1.2.0-win64/" (
    echo "Downloading libsndfile..." 
	cd thirdparty
	curl -OL https://github.com/libsndfile/libsndfile/releases/download/1.2.0/libsndfile-1.2.0-win64.zip
	7z x libsndfile-1.2.0-win64.zip -y
	rm libsndfile-1.2.0-win64.zip
	echo "Downloaded libsndfile." 
	cd ..
)

rem Clone Faust
if not exist "thirdparty/faust/" (
    echo "Downloading faust..." 
	cd thirdparty
	git clone https://github.com/grame-cncm/faust.git
	cd faust
	git checkout 28a5eacb0acbb80203b93ee71663d9a097536641
	echo "Downloaded faust." 
	cd ../..
)

cmake -Bbuild -DLIBFAUST_DIR="thirdparty/libfaust/win64/Release"
cmake --build build --config Release
cmake --build build --config Release --target install