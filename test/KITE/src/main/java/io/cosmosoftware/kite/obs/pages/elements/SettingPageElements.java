package io.cosmosoftware.kite.obs.pages.elements;

import java.util.HashMap;

public class SettingPageElements extends PageElements {


  // WINDOWS ------------------------------------------------------------------------------------------------
  private String generalSettingWIN = "/Window/Window[1]/Button[1]"; //
  private String streamSettingWIN = "/Window/Window[1]/Button[2]";//
  private String outputSettingWIN = "/Window/Window[1]/Button[3]";//
  private String audioSettingWIN = "/Window/Window[1]/Button[4]";//
  private String videoSettingWIN = "/Window/Window[1]/Button[5]";//
  private String hotkeysSettingWIN = "/Window/Window[1]/Button[6]";//
  private String advancedSettingWIN = "/Window/Window[1]/Button[7]";//

  private String OKBtnWIN = "/Window/Window[1]/Group/Button[1]"; //
  private String cancelBtnWIN = "/Window/Window[1]/Group/Button[2]";//
  private String applyBtnWIN = "/Window/Window[1]/Group/Button[3]";//

  // MAC ------------------------------------------------------------------------------------------------
  private String generalSettingMAC = "/AXApplication/AXWindow[0]/AXButton[0]"; //
  private String streamSettingMAC = "/AXApplication/AXWindow[0]/AXButton[1]";
  private String outputSettingMAC = "/AXApplication/AXWindow[0]/AXButton[2]";
  private String audioSettingMAC = "/AXApplication/AXWindow[0]/AXButton[3]";
  private String videoSettingMAC = "/AXApplication/AXWindow[0]/AXButton[4]";
  private String hotkeysSettingMAC = "/AXApplication/AXWindow[0]/AXButton[5]";
  private String advancedSettingMAC = "/AXApplication/AXWindow[0]/AXButton[6]";


  private String OKBtnMAC = "/AXApplication/AXWindow[0]/AXGroup[1]/AXButton[0]"; //
  private String cancelBtnMAC = "/AXApplication/AXWindow[0]/AXGroup[1]/AXButton[1]]";//
  private String applyBtnMAC = "/AXApplication/AXWindow[0]/AXGroup[1]/AXButton[2]";//

  public SettingPageElements() {
  }

  protected void getWindowsElements(HashMap<String, String> elements) {
    elements.put(PageElements.GENERAL_OPT, this.generalSettingWIN);
    elements.put(PageElements.STREAM_OPT, this.streamSettingWIN);
    elements.put(PageElements.OUTPUT_OPT, this.outputSettingWIN);
    elements.put(PageElements.AUDIO_OPT, this.audioSettingWIN);
    elements.put(PageElements.VIDEO_OPT, this.videoSettingWIN);
    elements.put(PageElements.HOTKEY_OPT, this.hotkeysSettingWIN);
    elements.put(PageElements.ADVANCED_OPT, this.advancedSettingWIN);
    elements.put(PageElements.OK_BTN, this.OKBtnWIN);
    elements.put(PageElements.CANCEL_BTN, this.cancelBtnWIN);
    elements.put(PageElements.APPLY_BTN, this.applyBtnWIN);
  }

  protected void getMacElements(HashMap<String, String> elements) {
    elements.put(PageElements.GENERAL_OPT, this.generalSettingMAC);
    elements.put(PageElements.STREAM_OPT, this.streamSettingMAC);
    elements.put(PageElements.OUTPUT_OPT, this.outputSettingMAC);
    elements.put(PageElements.AUDIO_OPT, this.audioSettingMAC);
    elements.put(PageElements.VIDEO_OPT, this.videoSettingMAC);
    elements.put(PageElements.HOTKEY_OPT, this.hotkeysSettingMAC);
    elements.put(PageElements.ADVANCED_OPT, this.advancedSettingMAC);
    elements.put(PageElements.OK_BTN, this.OKBtnMAC);
    elements.put(PageElements.CANCEL_BTN, this.cancelBtnMAC);
    elements.put(PageElements.APPLY_BTN, this.applyBtnMAC);
  }

  @Override
  protected void getLinuxElements(HashMap<String, String> elements) {

  }
}
