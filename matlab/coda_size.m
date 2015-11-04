function varargout = coda_size(varargin)
% CODA_SIZE  Retrieve the dimensions for an array in a product file.
%
%   DIMS = CODA_SIZE(CODA_FILE_ID, <DATA SPEC ARGS>) returns the
%   dimensions of the specified array. With coda_size you can check what
%   the dimensions of an array will be without the need to retrieve it
%   via coda_fetch. The returned dimension information is identical to
%   what would be returned by size(coda_fetch(...)) if coda_size and
%   coda_fetch had identical parameters. The last item of the data
%   specifier argument should point to an array.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('SIZE',varargin{:});
