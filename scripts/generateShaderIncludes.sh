#!/bin/sh

SHADERS_SRC="./src/renderer/gl/shaders/glsl"

echo "-- Generating shader includes"

if [ ! -d ./src/render/shaders ]; then
	mkdir ./src/render/shaders
fi

echo '#pragma once' > ./src/renderer/gl/shaders/Shaders.hpp
echo '#include <map>' >> ./src/renderer/gl/shaders/Shaders.hpp
echo 'static const std::map<std::string, std::string> SHADERS = {' >> ./src/renderer/gl/shaders/Shaders.hpp

for filename in `ls ${SHADERS_SRC}`; do
	echo "--	${filename}"
	
	{ echo 'R"#('; cat ${SHADERS_SRC}/${filename}; echo ')#"'; } > ./src/renderer/gl/shaders/${filename}.inc
	echo "{\"${filename}\"," >> ./src/renderer/gl/shaders/Shaders.hpp
	echo "#include \"./${filename}.inc\"" >> ./src/renderer/gl/shaders/Shaders.hpp
	echo "}," >> ./src/renderer/gl/shaders/Shaders.hpp
done

echo '};' >> ./src/renderer/gl/shaders/Shaders.hpp
