function varargout = coda_product_type(varargin)
% CODA_PRODUCT_TYPE  Retrieve product type of a product file.
%
%   PRODUCT_TYPE = CODA_PRODUCT_TYPE(CODA_FILE_ID) returns a string
%   containing the product type of the product.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open.
%
%   See also CODA_OPEN
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('PRODUCT_TYPE',varargin{:});
