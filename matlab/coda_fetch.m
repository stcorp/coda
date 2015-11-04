function varargout = coda_fetch(varargin)
% CODA_FETCH  Retrieve data from a product file.
%
%   DATA = CODA_FETCH(CODA_FILE_ID, <DATA SPEC ARGS>) reads the
%   specified data element from the product file. Instead of just
%   reading individual values, like strings, integers, doubles, etc. it
%   is also possible to read complete arrays or records of data. For
%   instance if 'pf' is a valid file handle, then you can read the
%   complete product by simply omitting the data specification argument:
%
%     mph = coda_fetch(pf)
%      
%   which gives you a MATLAB struct containing all the full product
%   content.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('FETCH',varargin{:});
