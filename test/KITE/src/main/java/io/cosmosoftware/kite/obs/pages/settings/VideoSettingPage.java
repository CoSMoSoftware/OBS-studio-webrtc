package io.cosmosoftware.kite.obs.pages.settings;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;

public class VideoSettingPage extends SettingPage {

  // WINDOWS ------------------------------------------------------------------------------------------------
  private static String baseResolutionWIN = "/Window/Window[1]/Custom/Group/ComboBox[1]";
  private static String outputResolutionWIN = "/Window/Window[1]/Custom/Group/ComboBox[2]";
  private static String downscaledFilterWIN = "/Window/Window[1]/Custom/Group/ComboBox[3]";
  private static String FpsWIN = "/Window/Window[1]/Custom/Group/Custom/Group/ComboBox";

  // MAC ------------------------------------------------------------------------------------------------
  private static String baseResolutionMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXPopUpButton[0]";
  private static String outputResolutionMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXPopUpButton[1]";
  private static String downscaledFilterMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXPopUpButton[2]";
  private static String FpsMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXGroup/AXPopUpButton";

  public VideoSettingPage(Runner runner) {
    super(runner);
  }

  //todo : verify tag name call

  @Override
  protected String getSettingOptionName() {
    return PageElements.VIDEO_OPT;
  }

  public String getBaseResolution() throws KiteInteractionException {
    if (this.os.equals("WINDOWS")) {
      return this.getElement(PageElements.BASE_RESOLUTION).getText();
    } else {
      return this.getElement(PageElements.BASE_RESOLUTION).getTagName();
    }
  }

  public String getOutputResolution() throws KiteInteractionException {
    if (this.os.equals("WINDOWS")) {
      return this.getElement(PageElements.OUTPUT_RESOLUTION).getText();
    } else {
      return this.getElement(PageElements.OUTPUT_RESOLUTION).getTagName();
    }
  }

  public String getDownscaledFilter() throws KiteInteractionException {
    if (this.os.equals("WINDOWS")) {
      return this.getElement(PageElements.DOWNSCALED_FILTER).getText();
    } else {
      return this.getElement(PageElements.DOWNSCALED_FILTER).getTagName();
    }
  }

  public String getFps() throws KiteInteractionException {
    if (this.os.equals("WINDOWS")) {
      return this.getElement(PageElements.FPS).getText();
    } else {
      return this.getElement(PageElements.FPS).getTagName();
    }
  }

  @Override
  protected void addWindowsElements() {
    this.elements.put(PageElements.BASE_RESOLUTION, baseResolutionWIN);
    this.elements.put(PageElements.OUTPUT_RESOLUTION, outputResolutionWIN);
    this.elements.put(PageElements.DOWNSCALED_FILTER, downscaledFilterWIN);
    this.elements.put(PageElements.FPS, FpsWIN);

  }

  @Override
  protected void addMacElements() {
    this.elements.put(PageElements.BASE_RESOLUTION, baseResolutionMAC);
    this.elements.put(PageElements.OUTPUT_RESOLUTION, outputResolutionMAC);
    this.elements.put(PageElements.DOWNSCALED_FILTER, downscaledFilterMAC);
    this.elements.put(PageElements.FPS, FpsMAC);
  }

  @Override
  protected void addLinuxElements() {

  }

}
