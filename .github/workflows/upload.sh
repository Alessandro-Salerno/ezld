git rm -rf *.md
git rm -rf .github
git rm -rf include
git rm -rf src
git rm -rf .gitattributes
git rm -rf .gitignore
git rm -rf .clang-format
git rm -rf Makefile
git rm -rf ./bin
git rm -rf ./plugins
git rm -rf compile_flags.txt

git commit -m "new bundle"
git push --set-upstream origin latest-build --force
