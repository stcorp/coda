function varargout = coda_fieldnames(varargin)
% CODA_FIELDNAMES  Retrieve a list of fieldnames for a record in a
%   product file.
%
%   FIELDNAMES = CODA_FIELDNAMES(CODA_FILE_ID, <DATA SPEC ARGS>) returns
%   a cellarray of strings of fieldnames for the struct that would be
%   returned if coda_fetch would have been called with the same
%   arguments. The last item of the data specifier argument should point
%   to a record.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('FIELDNAMES',varargin{:});
