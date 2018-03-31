function mv=is_adtomv(ticks)
% USAGE: MV=IS_ADTOMV(TICKS);
%
%	IS_ADTOMV converts waveforms from AD ticks (waveforms returned from
%	IS_RECORD are in AD ticks) to mV.
%
%   TICKS is an MxN array of N waveforms of M samples each.
%   MV is TICKS, converted from AD ticks to mV.
%
% SEE ALSO: IS_RECORD


% Get the conversion factors
[tomv offset]=iserver('ad_convert');

n=size(ticks,2);
for i=1:n,
	newbuf(:,i)=(ticks(:,i) + offset(1,i)*(ones(size(ticks,1),1)))*tomv(1,i);
end




