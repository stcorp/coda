/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.1.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package nl.stcorp.coda;

public enum SpecialTypeEnum {
  coda_special_no_data,
  coda_special_vsf_integer,
  coda_special_time,
  coda_special_complex;

  public final int swigValue() {
    return swigValue;
  }

  public static SpecialTypeEnum swigToEnum(int swigValue) {
    SpecialTypeEnum[] swigValues = SpecialTypeEnum.class.getEnumConstants();
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (SpecialTypeEnum swigEnum : swigValues)
      if (swigEnum.swigValue == swigValue)
        return swigEnum;
    throw new IllegalArgumentException("No enum " + SpecialTypeEnum.class + " with value " + swigValue);
  }

  @SuppressWarnings("unused")
  private SpecialTypeEnum() {
    this.swigValue = SwigNext.next++;
  }

  @SuppressWarnings("unused")
  private SpecialTypeEnum(int swigValue) {
    this.swigValue = swigValue;
    SwigNext.next = swigValue+1;
  }

  @SuppressWarnings("unused")
  private SpecialTypeEnum(SpecialTypeEnum swigEnum) {
    this.swigValue = swigEnum.swigValue;
    SwigNext.next = this.swigValue+1;
  }

  private final int swigValue;

  private static class SwigNext {
    private static int next = 0;
  }
}

