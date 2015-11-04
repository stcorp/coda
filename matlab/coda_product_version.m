function varargout = coda_product_version(varargin)
% CODA_PRODUCT_VERSION  Retrieve product version for a product file.
%
%   PRODUCT_VERSION = CODA_PRODUCT_VERSION(CODA_FILE_ID) returns an integer
%   denoting the product version, maintained by CODA to be able to
%   distinguish between different versions of product specifications.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open.
%
%   See also CODA_OPEN
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('PRODUCT_VERSION',varargin{:});
