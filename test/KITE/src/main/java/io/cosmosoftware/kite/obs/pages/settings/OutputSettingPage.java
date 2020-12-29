package io.cosmosoftware.kite.obs.pages.settings;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.Keys;
import org.openqa.selenium.WebElement;

public class OutputSettingPage extends SettingPage {

  // WINDOWS ------------------------------------------------------------------------------------------------
  private static String outputModeOptionWIN = "/Window/Window[1]/Custom/Group/Group/Group/Group/Group/RadioButton";
  private static String outputGroupElementWIN = "/Window/Window[1]/Custom/Group/Group/Group/Group/Custom/Group/Group";
  private static String videoBitrateFieldWIN = "/Window/Window[1]/Custom/Group/Group/Group/Group/Custom/Group/Group[1]/Edit[1]";
  private static String audioBitrateFieldWIN = "/Window/Window[1]/Custom/Group/Group/Group/Group/Custom/Group/Group[1]/ComboBox[1]";
  private static String endcoderFieldWIN = "/Window/Window[1]/Custom/Group/Group/Group/Group/Custom/Group/Group[1]/ComboBox[2]";

  // MAC ------------------------------------------------------------------------------------------------
  private static String outputModeOptionMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXRadioButton";
  private static String outputGroupElementMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXGroup/AXGroup";
  private static String videoBitrateFieldMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup/AXGroup/AXTextField[0]";
  private static String audioBitrateFieldMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup/AXGroup/AXPopUpButton[0]";
  private static String endcoderFieldMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup/AXGroup/AXPopUpButton[1]";

  public OutputSettingPage(Runner runner) {
    super(runner);
  }

  public List<String> getSettingGroupLabels() {
    List<String> res = new ArrayList<>();
    if (this.os.equals("WINDOWS")) {
      List<WebElement> groups = this.webDriver
          .findElements(By.xpath(elements.get(PageElements.OUTPUT_GROUP_ELEMENT)));
      for (WebElement group : groups) {
        res.add(group.getAttribute("Name"));
      }
    } else {
      List<WebElement> groups = this
          .getElementByXpath(elements.get(PageElements.OUTPUT_GROUP_ELEMENT));
      for (WebElement group : groups) {
        res.add(group.getTagName());
      }
    }
    return res;
  }

  public String getVideoBitrate() throws KiteInteractionException {
    return getElement(PageElements.OUTPUT_VIDEO_BITRATE).getText();
  }

  public void changeVideoBitrate(String bitrate) throws KiteInteractionException {
    if (!this.os.equals("MAC")) {
      try {
        getElement(PageElements.OUTPUT_VIDEO_BITRATE).sendKeys(Keys.HOME,
            Keys.DELETE, Keys.DELETE, Keys.DELETE, Keys.DELETE, Keys.DELETE, Keys.DELETE);
      } catch (Exception e) {
        throw new KiteInteractionException("Could not delete existing birate value");
      }
      sendKeys(getElement(PageElements.OUTPUT_VIDEO_BITRATE), bitrate);
    }

  }

  public boolean audioBitrateEnabled() throws KiteInteractionException {
    return getElement(PageElements.OUTPUT_AUDIO_BITRATE).isEnabled();
  }

  public List<String> getOutputModes() throws KiteInteractionException {
    // click(getElement(PageElements.OUTPUT_MODES));
    List<String> res = new ArrayList<>();
    if (this.os.equals("WINDOWS")) {
      List<WebElement> outputModes = webDriver
          .findElements(By.xpath(this.elements.get(PageElements.OUTPUT_MODES_OPTIONS)));
      for (WebElement outputMode : outputModes) {
        res.add(outputMode.getAttribute("Name"));
      }
    } else if (this.os.equals("MAC")) {
      // webDriver.findElements(..) doesn't work on MAC
      List<WebElement> outputModes = this
          .getElementByXpath(this.elements.get(PageElements.OUTPUT_MODES_OPTIONS));
      for (WebElement outputMode : outputModes) {
        res.add(outputMode.getTagName());
      }
    } else {
      // LINUX
    }
    return res;
  }

  public boolean getEndcoderFieldAvailibility() throws KiteInteractionException {
    return this.getElement(PageElements.ENDCODER).isEnabled();
  }

  @Override
  protected String getSettingOptionName() {
    return PageElements.OUTPUT_OPT;
  }

  @Override
  protected void addWindowsElements() {
    this.elements.put(PageElements.OUTPUT_MODES_OPTIONS, outputModeOptionWIN);
    this.elements.put(PageElements.OUTPUT_GROUP_ELEMENT, outputGroupElementWIN);
    this.elements.put(PageElements.OUTPUT_VIDEO_BITRATE, videoBitrateFieldWIN);
    this.elements.put(PageElements.OUTPUT_AUDIO_BITRATE, audioBitrateFieldWIN);
    this.elements.put(PageElements.ENDCODER, endcoderFieldWIN);
  }

  @Override
  protected void addMacElements() {
    this.elements.put(PageElements.OUTPUT_MODES_OPTIONS, outputModeOptionMAC);
    this.elements.put(PageElements.OUTPUT_GROUP_ELEMENT, outputGroupElementMAC);
    this.elements.put(PageElements.OUTPUT_VIDEO_BITRATE, videoBitrateFieldMAC);
    this.elements.put(PageElements.OUTPUT_AUDIO_BITRATE, audioBitrateFieldMAC);
    this.elements.put(PageElements.ENDCODER, endcoderFieldMAC);
  }

  @Override
  protected void addLinuxElements() {

  }

}
