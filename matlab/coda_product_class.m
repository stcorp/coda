function varargout = coda_product_class(varargin)
% CODA_PRODUCT_CLASS  Retrieve product class of a product file.
%
%   PRODUCT_CLASS = CODA_PRODUCT_CLASS(CODA_FILE_ID) returns a string
%   containing the product class of the product.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open.
%
%   See also CODA_OPEN
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('PRODUCT_CLASS',varargin{:});
