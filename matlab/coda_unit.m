function varargout = coda_unit(varargin)
% CODA_UNIT  Retrieve unit information.
%
%   UNIT = CODA_UNIT(CODA_FILE_ID, <DATA SPEC ARGS>) returns a string
%   containing the unit information which is stored in the data
%   dictionary for the specified data element.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('UNIT',varargin{:});
