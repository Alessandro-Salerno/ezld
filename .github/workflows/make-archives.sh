git config --local user.email "41898282+github-actions[bot]@users.noreply.github.com"
git config --local user.name "github-actions[bot]"
git checkout --orphan latest-build

mkdir $1
mkdir ezld
cp ./bin/ezld ./ezld/ezld
tar -cvzf ./$1/ezld-$1.tar.gz ./ezld
rm -rf ./ezld

git add $1/
