package io.cosmosoftware.kite.obs.pages.elements;

import java.util.HashMap;

public class MainPageElements extends PageElements {

  // WINDOWS============================================================
  private String buttonWIN = "/Window/Window[5]/Group/Button";
  private String addSourceButton = "/Window/Window[5]/Group/Button";


  private String startStreamBtnWIN = "/Window/Window[5]/Group/CheckBox[1]";//
  private String settingButtonWIN = "/Window/Window[5]/Group/Button[1]";//

  private String dropFrameStatusWIN = "/Window/StatusBar/Text[1]";
  private String statusTextWIN = "/Window/StatusBar/Text";
  private String liveStatusWIN = "/Window/StatusBar/Text[2]";
  private String recStatusWIN = "/Window/StatusBar/Text[3]";
  private String cpuStatusWIN = "/Window/StatusBar/Text[4]";
  private String getStatsWIN = "/Window/StatusBar/Group/Edit[1]";

  private String exitBtnWIN = "/Window/Window[5]/Group/Button[2]"; //

  // MAC================================================================
  private String buttonMAC = "/AXApplication/AXWindow/AXButton";
  private String startStreamBtnMAC = "/AXApplication/AXWindow/AXCheckBox[0]";//
  private String settingButtonMAC = "/AXApplication/AXWindow/AXButton[16]";//
  private String statusTextMAC = "/AXApplication/AXWindow/AXStaticText[0]/AXStaticText[2]";//
  private String dropFrameStatusMAC = "/AXApplication/AXWindow/AXStaticText[0]/AXStaticText[1]";
  private String liveStatusMAC = "/AXApplication/AXWindow/AXStaticText[0]/AXStaticText[2]";
  private String cpuStatusMAC = "/AXApplication/AXWindow/AXStaticText[0]/AXStaticText[3]";
  private String recStatusMAC = "/AXApplication/AXWindow/AXStaticText[0]/AXStaticText[4]";
  private String getStatsMAC = "/AXApplication/AXWindow/AXStaticText[0]/AXTextArea[0]";
  private String exitBtnMAC = "/AXApplication/AXWindow/AXButton[17]";


  public MainPageElements() {

  }

  public HashMap<String, String> populateElement(String os) {
    HashMap<String, String> elements = new HashMap<>();

    if (os.equals("WINDOWS")) {
      getWindowsElements(elements);
    } else if (os.equals("MAC")) {
      getMacElements(elements);
    } else {
      getLinuxElements(elements);
    }
    return elements;
  }

  protected void getWindowsElements(HashMap<String, String> elements) {
    elements.put(PageElements.MAIN_PAGE_BUTTON, this.buttonWIN);
    elements.put(PageElements.MAIN_PAGE_START_STREAMING_BTN, this.startStreamBtnWIN);
    elements.put(PageElements.MAIN_PAGE_SETTING_BTN, this.settingButtonWIN);
    elements.put(PageElements.MAIN_PAGE_STATUS_TEXT, this.statusTextWIN);
    elements.put(PageElements.MAIN_PAGE_FRAME_STATUS, this.dropFrameStatusWIN);
    elements.put(PageElements.MAIN_PAGE_LIVE_STATUS, this.liveStatusWIN);
    elements.put(PageElements.MAIN_PAGE_REC_STATUS, this.recStatusWIN);
    elements.put(PageElements.MAIN_PAGE_CPU_STATUS, this.cpuStatusWIN);
    elements.put(PageElements.MAIN_PAGE_STATS, this.getStatsWIN);
    elements.put(PageElements.MAIN_PAGE_EXIT_BTN, this.exitBtnWIN);
  }

  protected void getMacElements(HashMap<String, String> elements) {
    elements.put(PageElements.MAIN_PAGE_BUTTON, this.buttonMAC);
    elements.put(PageElements.MAIN_PAGE_START_STREAMING_BTN, this.startStreamBtnMAC);
    elements.put(PageElements.MAIN_PAGE_SETTING_BTN, this.settingButtonMAC);
    elements.put(PageElements.MAIN_PAGE_STATUS_TEXT, this.statusTextMAC);
    elements.put(PageElements.MAIN_PAGE_FRAME_STATUS, this.dropFrameStatusMAC);
    elements.put(PageElements.MAIN_PAGE_LIVE_STATUS, this.liveStatusMAC);
    elements.put(PageElements.MAIN_PAGE_REC_STATUS, this.recStatusMAC);
    elements.put(PageElements.MAIN_PAGE_CPU_STATUS, this.cpuStatusMAC);
    elements.put(PageElements.MAIN_PAGE_STATS, this.getStatsMAC);
    elements.put(PageElements.MAIN_PAGE_EXIT_BTN, this.exitBtnMAC);
  }

  protected void getLinuxElements(HashMap<String, String> elements) {
  }

}
