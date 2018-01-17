/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.microsoft.adaptivecards.objectmodel;

public class Fact {
  private transient long swigCPtr;
  private transient boolean swigCMemOwn;

  protected Fact(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Fact obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        AdaptiveCardObjectModelJNI.delete_Fact(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public Fact() {
    this(AdaptiveCardObjectModelJNI.new_Fact__SWIG_0(), true);
  }

  public Fact(String title, String value) {
    this(AdaptiveCardObjectModelJNI.new_Fact__SWIG_1(title, value), true);
  }

  public String Serialize() {
    return AdaptiveCardObjectModelJNI.Fact_Serialize(swigCPtr, this);
  }

  public SWIGTYPE_p_Json__Value SerializeToJsonValue() {
    return new SWIGTYPE_p_Json__Value(AdaptiveCardObjectModelJNI.Fact_SerializeToJsonValue(swigCPtr, this), true);
  }

  public String GetTitle() {
    return AdaptiveCardObjectModelJNI.Fact_GetTitle(swigCPtr, this);
  }

  public void SetTitle(String value) {
    AdaptiveCardObjectModelJNI.Fact_SetTitle(swigCPtr, this, value);
  }

  public String GetValue() {
    return AdaptiveCardObjectModelJNI.Fact_GetValue(swigCPtr, this);
  }

  public void SetValue(String value) {
    AdaptiveCardObjectModelJNI.Fact_SetValue(swigCPtr, this, value);
  }

  public static Fact Deserialize(ElementParserRegistration elementParserRegistration, ActionParserRegistration actionParserRegistration, SWIGTYPE_p_Json__Value root) {
    long cPtr = AdaptiveCardObjectModelJNI.Fact_Deserialize(ElementParserRegistration.getCPtr(elementParserRegistration), elementParserRegistration, ActionParserRegistration.getCPtr(actionParserRegistration), actionParserRegistration, SWIGTYPE_p_Json__Value.getCPtr(root));
    return (cPtr == 0) ? null : new Fact(cPtr, true);
  }

  public static Fact DeserializeFromString(ElementParserRegistration elementParserRegistration, ActionParserRegistration actionParserRegistration, String jsonString) {
    long cPtr = AdaptiveCardObjectModelJNI.Fact_DeserializeFromString(ElementParserRegistration.getCPtr(elementParserRegistration), elementParserRegistration, ActionParserRegistration.getCPtr(actionParserRegistration), actionParserRegistration, jsonString);
    return (cPtr == 0) ? null : new Fact(cPtr, true);
  }

}
