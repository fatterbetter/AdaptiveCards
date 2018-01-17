/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.microsoft.adaptivecards.objectmodel;

public class TextConfig {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected TextConfig(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(TextConfig obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        AdaptiveCardObjectModelJNI.delete_TextConfig(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setWeight(TextWeight value) {
    AdaptiveCardObjectModelJNI.TextConfig_weight_set(swigCPtr, this, value.swigValue());
  }

  public TextWeight getWeight() {
    return TextWeight.swigToEnum(AdaptiveCardObjectModelJNI.TextConfig_weight_get(swigCPtr, this));
  }

  public void setSize(TextSize value) {
    AdaptiveCardObjectModelJNI.TextConfig_size_set(swigCPtr, this, value.swigValue());
  }

  public TextSize getSize() {
    return TextSize.swigToEnum(AdaptiveCardObjectModelJNI.TextConfig_size_get(swigCPtr, this));
  }

  public void setColor(ForegroundColor value) {
    AdaptiveCardObjectModelJNI.TextConfig_color_set(swigCPtr, this, value.swigValue());
  }

  public ForegroundColor getColor() {
    return ForegroundColor.swigToEnum(AdaptiveCardObjectModelJNI.TextConfig_color_get(swigCPtr, this));
  }

  public void setIsSubtle(boolean value) {
    AdaptiveCardObjectModelJNI.TextConfig_isSubtle_set(swigCPtr, this, value);
  }

  public boolean getIsSubtle() {
    return AdaptiveCardObjectModelJNI.TextConfig_isSubtle_get(swigCPtr, this);
  }

  public void setWrap(boolean value) {
    AdaptiveCardObjectModelJNI.TextConfig_wrap_set(swigCPtr, this, value);
  }

  public boolean getWrap() {
    return AdaptiveCardObjectModelJNI.TextConfig_wrap_get(swigCPtr, this);
  }

  public void setMaxWidth(long value) {
    AdaptiveCardObjectModelJNI.TextConfig_maxWidth_set(swigCPtr, this, value);
  }

  public long getMaxWidth() {
    return AdaptiveCardObjectModelJNI.TextConfig_maxWidth_get(swigCPtr, this);
  }

  public static TextConfig Deserialize(SWIGTYPE_p_Json__Value json, TextConfig defaultValue) {
    return new TextConfig(AdaptiveCardObjectModelJNI.TextConfig_Deserialize(SWIGTYPE_p_Json__Value.getCPtr(json), TextConfig.getCPtr(defaultValue), defaultValue), true);
  }

  public TextConfig() {
    this(AdaptiveCardObjectModelJNI.new_TextConfig(), true);
  }

}
