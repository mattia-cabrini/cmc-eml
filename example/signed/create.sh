cat test.txt | ./cmc-eml

cat ../../ignore/pass | gpg \
	--pinentry-mode loopback \
	--passphrase-fd 0 \
	--digest-algo SHA256 \
	--detach-sign \
	-o test.eml.asc \
	--yes \
	--armor \
	-u dev@mattiacabrini.com \
	test.eml

CR=$(printf '\r')
sed -i "s/\$/$CR/" test.eml.asc

cat test_signed.txt | ./cmc-eml
