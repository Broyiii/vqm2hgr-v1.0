# vqm2hgr
 Change vqm file of titan23 into hgr file.

```
cd ./main
g++ main.cpp -O3 -o main
./main ./work/carpat/carpat_stratixiv.vqm
```

update in 2025.01.24
Support following situation : 
1. {xxx,abc[7:5],xxx}
2. wire net[7:0]; .a(net)