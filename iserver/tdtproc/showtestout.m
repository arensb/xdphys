1;

[fileno,err]=fopen('left_da.out','r');
[ldata,count]=fread(fileno,inf,'short');
fclose(fileno);
[fileno,err]=fopen('right_da.out','r');
[rdata,count]=fread(fileno,inf,'short');
fclose(fileno);

subplot(2,1,1);
plot(ldata);
subplot(2,1,2);
plot(rdata);
