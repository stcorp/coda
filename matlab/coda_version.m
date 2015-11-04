function varargout = coda_version(varargin)
% CODA_VERSION  Get version number of CODA.
%
%   VERSION = CODA_VERSION returns the version number of CODA.
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('VERSION',varargin{:});
