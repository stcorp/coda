function varargout = coda_fieldavailable(varargin)
% CODA_FIELDAVAILABLE  Find out whether a dynamically available record field
%   is available or not.
%
%   IS_AVAILABLE = CODA_FIELDAVAILABLE(CODA_FILE_ID, <DATA SPEC ARGS>)
%   returns 1 if the record field is available and 0 if it is not. The last
%   item of the data specifier argument should point to a record field.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('FIELDAVAILABLE',varargin{:});
