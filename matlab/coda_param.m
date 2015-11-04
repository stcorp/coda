% CODA_PARAM A description of the data specification argument list
%
%   Several functions from the CODA MATLAB interface take a data
%   specification argument list to specify a certain data element in a
%   product file. This argument list is a comma separated list of names
%   and identifiers that allow you to walk to a certain point in a
%   product. The list can be left empty if you want to point to the
%   root of the product. There are two types of arguments you can use
%   in the list of arguments:
%
%    - Array index: You need to provide an array index that has the
%      same number of elements as there are dimensions in the array
%      that you are refering to. So if the array is two dimensional you
%      have to pass indices for both dimensions like [4 5]. If the
%      array is one dimensional (like the datasets) you can just
%      provide a single index value (without the '[]') to get to the
%      k-th element. You can use coda_size to check the number of
%      dimensions of an array and the size of each dimension.
%    - Field name: To go to a certain field inside the record that you
%      are pointing to, you can provide the field name as a string
%      (which is case sensitive, so you should make sure that the name
%      is exactly the same as is returned by coda_fieldnames).
%
%   Note that all indices inside the data specification argument list
%   go from 1..N (and not from 0..(N-1)).
%
%   For the <code>coda_fetch</code> function there is an additional
%   feature. If you provide a -1 for one or more of the dimensions of an
%   array you will fetch all elements in the specified dimension(s).
%   For example, with coda_fetch(pf,'datasetname',-1,'dsr_time') you can
%   fetch all dsr_time values for all measurements into a single array.
%
%   See also CODA_CLASS, CODA_EVAL, CODA_FETCH, CODA_FIELDNAMES, CODA_SIZE
%
