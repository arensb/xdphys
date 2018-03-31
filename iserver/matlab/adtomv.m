function newbuf=adtomv(oldbuf)
% USAGE: NEWBUF=ADTOMV(OLDBUF);
%
%     where
%
%        OLDBUF is an MxN array of N AD waveforms of M samples each
%        NEWBUF is OLDBUF converted from adticks to mV
%
%


% Get the conversion factors
[tomv offset]=iserver('ad_convert');

n=size(oldbuf,2);
for i=1:n,
	newbuf(:,i)=(oldbuf(:,i) + offset(1,i)*(ones(size(oldbuf,1),1)))*tomv(1,i);
end




