function out=phasesine(freq,fc,itd_start,deg_speed,itd_end,us_per_deg)
% out=phasesine(freq,fc,itd_start,deg_speed,itd_end,us_per_deg)
%

if (nargin==5),
	us_per_deg=2;
end

nsamps=((itd_end-itd_start)/(deg_speed*us_per_deg))*fc;
samps=[0:nsamps]/nsamps;
usec_range=([0:nsamps]*((itd_end-itd_start)/nsamps))+itd_start;

out(:,1)=zeros(size(samps'));
out(:,2)=zeros(size(samps'));
for i=1:length(freq),

	deg_range=((usec_range/1e6)*freq(i))*360;

	temp(:,1)=sin(2*pi*freq(i)*samps'+2*pi*deg_range'/360);
	temp(:,2)=sin(2*pi*freq(i)*samps');
	out=out+temp;
end
out(:,1)=32000*(out(:,1)/max(out(:,1)));
out(:,2)=32000*(out(:,2)/max(out(:,2)));

out=floor(out);
