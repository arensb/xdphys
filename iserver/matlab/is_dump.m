function [da,ad]=is_dump()
% USAGE: [DABUF ADBUF]=IS_DUMP;
%
%        Retrieve the current contents of the AD and DA buffers, and
%        return them.

[da ad]=iserver('dump');

