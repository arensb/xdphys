function is_shutdown()
% USAGE: IS_SHUTDOWN;
%     
%      Kill the tdtproc process, and stop data acquisition.
%

iserver('shutdown')

clear global is_info

