// Copyright (C) 2007-2023 S[&]T, The Netherlands.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

package nl.stcorp.coda;

/**
 * CODA Expression class.
 * 
 * This class represents instances of CODA Expressions and provides methods to
 * manipulate retrieve information about the expression.
 * 
 */

public class Expression
{
    private SWIGTYPE_p_coda_expression_struct expr;


    /**
     * Returns the name of an expression type.
     * 
     * @param type
     *            CODA expression type
     * @return if the type is known a string containing the name of the
     *         type, otherwise the string "unknown".
     */
    public static String getTypeName(ExpressionTypeEnum type)
    {
        return codac.expression_get_type_name(type);
    }

    /**
     * Create a new CODA expression object by parsing a string containing a CODA
     * expression.
     * 
     * The string should contain a valid CODA expression. The returned
     * expression object should be cleaned up using the delete() method after it
     * has been used.
     * 
     * @param exprString
     *            A string containing the string representation of the CODA
     *            expression
     * @throws CodaException
     *             If an error occurred.
     * 
     */
    public Expression(String exprString) throws CodaException
    {
        this.expr = new SWIGTYPE_p_coda_expression_struct();
        codac.expression_from_string(exprString, this.expr);
    }


    /**
     * Delete the CODA expression object.
     * 
     * This will invalidate the Expression object. Note: the results of
     * continuing to use an Expr object after this method has been called are
     * undefined. Currently, no internal state is maintained by the object
     * itself.
     * 
     */
    public void delete()
    {
        if (this.expr != null)
            codac.expression_delete(this.expr);
        this.expr = null;
    }


    /**
     * Retrieve the result type of a CODA expression.
     * 
     * @return The result type of the CODA expression.
     * @throws CodaException
     *             If an error occurred.
     */
    public ExpressionTypeEnum getType() throws CodaException
    {
        int expression_type[] = new int[1];
        codac.expression_get_type(this.expr, expression_type);
        return ExpressionTypeEnum.swigToEnum(expression_type[0]);
    }


    /**
     * Return whether an expression is constant or not.
     * 
     * An expression is constant if it does not depend on the contents of a
     * product and if the expression evaluation function can be called with
     * cursor=null.
     * 
     * @return true if the expression is constant, false if not.
     */
    public boolean isConstant()
    {
        return (codac.expression_is_constant(this.expr) == 1);
    }


    /**
     * Return whether an expression is equal to another expression or not.
     * 
     * @return true if the expression matches the given expression, false if not.
     */
    public boolean isEqual(Expression expression)
    {
        return (codac.expression_is_equal(this.expr, expression.expr) == 1);
    }


    /**
     * Evaluate a boolean expression. Shortcut for evalBool(null).
     * 
     * @return The resulting boolean value.
     * @throws CodaException
     *             If an error occurred.
     */
    public boolean evalBool() throws CodaException
    {
        return evalBool(null);
    }


    /**
     * Evaluate a boolean expression. The expression object should be a
     * coda_expression_bool expression. The function will evaluate the
     * expression at the given cursor position and return the resulting boolean
     * value.
     * 
     * @param cursor
     *            Cursor pointing to a location in the product where the boolean
     *            expression should be evaluated (can be NULL for constant
     *            expressions).
     * @return The resulting boolean value.
     * @throws CodaException
     *             If an error occurred.
     */
    public boolean evalBool(Cursor cursor) throws CodaException
    {
        int dst[] = new int[1];
        if (cursor != null)
            codac.expression_eval_bool(this.expr,
                    cursor.getSwigRepresentation(),
                    dst);
        else
            codac.expression_eval_bool(this.expr, null, dst);
        return dst[0] == 1;
    }


    /**
     * Evaluate an integer expression. Shortcut for evalInteger(null).
     * 
     * @return The resulting integer value (note that the the actual data type
     *         is 'long').
     * @throws CodaException
     *             If an error occurred.
     */
    public long evalInteger() throws CodaException
    {
        return evalInteger(null);
    }


    /**
     * Evaluate an integer expression. The expression object should be a
     * coda_expression_integer expression. The function will evaluate the
     * expression at the given cursor position and return the resulting integer
     * value.
     * 
     * @param cursor
     *            Cursor pointing to a location in the product where the boolean
     *            expression should be evaluated (can be NULL for constant
     *            expressions).
     * @return The resulting integer value (note that the the actual data type
     *         is 'long').
     * @throws CodaException
     *             If an error occurred.
     */
    public long evalInteger(Cursor cursor) throws CodaException
    {
        long dst[] = new long[1];
        if (cursor != null)
            codac.expression_eval_integer(this.expr,
                    cursor.getSwigRepresentation(),
                    dst);
        else
            codac.expression_eval_integer(this.expr, null, dst);
        return dst[0];
    }


    /**
     * Evaluate a floating point expression. Shortcut for evalFloat(null).
     * 
     * @return The resulting floating point value.
     * @throws CodaException
     *             If an error occurred.
     */
    public double evalFloat() throws CodaException
    {
        return evalFloat(null);
    }


    /**
     * Evaluate a floating point expression. The function will evaluate the
     * expression at the given cursor position and return the resulting floating
     * point value. The expression object should be a coda_expression_float
     * expression.
     * 
     * @param cursor
     *            Cursor pointing to a location in the product where the boolean
     *            expression should be evaluated (can be NULL for constant
     *            expressions).
     * @return The resulting floating point value.
     * @throws CodaException
     *             If an error occurred.
     */
    public double evalFloat(Cursor cursor) throws CodaException
    {
        double dst[] = new double[1];
        if (cursor != null)
            codac.expression_eval_float(this.expr,
                    cursor.getSwigRepresentation(),
                    dst);
        else
            codac.expression_eval_float(this.expr, null, dst);
        return dst[0];
    }


    /**
     * Evaluate a string expression. Shortcut for evalString(null).
     * 
     * @return The resulting string value.
     * @throws CodaException
     *             If an error occurred.
     */
    public String evalString() throws CodaException
    {
        return evalString(null);
    }


    /**
     * Evaluate a string expression. The function will evaluate the expression
     * at the given cursor position (if provided) and return the resulting
     * string. If a string is returned then it will be zero terminated. However,
     * in the case where the string itself also contains zero characters,
     * strlen() can not be used and the \c length parameter will give the actual
     * string length of \c value. The expression object should be a
     * coda_expression_string expression
     * 
     * @param cursor
     *            Cursor pointing to a location in the product where the boolean
     *            expression should be evaluated (can be NULL for constant
     *            expressions).
     * @return The resulting string value.
     * @throws CodaException
     *             If an error occurred.
     */
    public String evalString(Cursor cursor) throws CodaException
    {
        String value[] = {""};
        int length[] = new int[1];
        if (cursor != null)
            codac.expression_eval_string(this.expr,
                    cursor.getSwigRepresentation(),
                    value,
                    length);
        else
            codac.expression_eval_string(this.expr, null, value, length);
        return value[0];
    }


    /**
     * Evaluate a node expression. The function will moves the cursor to a
     * different position in a product based on the node expression. The
     * expression object should be a coda_expr_node expression.
     * 
     * @param cursor
     *            Cursor pointing to a location in the product where the boolean
     *            expression should be evaluated.
     * @throws CodaException
     *             If an error occurred.
     */
    public void evalNode(Cursor cursor) throws CodaException
    {
        codac.expression_eval_node(this.expr, cursor.getSwigRepresentation());
    }
}
