package io.cosmosoftware.kite.obs.pages.settings;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.Keys;
import org.openqa.selenium.WebElement;

public class StreamSettingPage extends SettingPage {

  // WINDOWS ------------------------------------------------------------------------------------------------
  private static String streamTypeOptionWIN = "/Window/Window[1]/Custom/Group/Group/ComboBox";//
  private static String streamTypeOptionItemWIN = "/Window/Window[1]/Custom/Group/Group/ComboBox/List/ListItem";//

  private static String rtmpServerNameTextFieldWIN = "/Window/Window[1]/Custom/Group/Custom/Group/Custom/Group/Edit";//
  private static String rtmpStreamKeyTextFieldWIN = "/Window/Window[1]/Custom/Group/Custom/Group/Group/Edit";//

  private static String streamNameTextEditWIN = "/Window/Window[1]/Custom/Group/Custom/Group/Edit[1]"; //
  private static String publishingTokenTextEditWIN = "/Window/Window[1]/Custom/Group/Custom/Group/Group/Edit[1]"; //

  private static String codecWIN = "/Window/Window[1]/Custom/Group/Custom/Group/ComboBox"; //
  private static String codecListItemWIN = "/Window/Window[1]/Custom/Group/Custom/Group/ComboBox/List/ListItem";//

  // MAC ------------------------------------------------------------------------------------------------
  private static String streamTypeOptionMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXRadioButton";
  private static String streamTypeOptionItemMAC = "/Window/Window[1]/Pane/Custom/Custom/ComboBox/List/ListItem";

  private static String rtmpServerNameTextFieldMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup/AXGroup/AXTextField";
  private static String rtmpStreamKeyTextFieldMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup/AXTextField";

  private static String streamNameTextEditMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup/AXTextField[0]";
  private static String publishingTokenTextEditMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup/AXTextField[1]";

  private static String codecMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXGroup[0]/AXGroup[0]/AXRadioButton";
  private static String codecListItemMAC = "/Window/Window[1]/Pane/Custom/Pane/Custom/ComboBox/List/ListItem";

  public StreamSettingPage(Runner runner) {
    super(runner);
  }

  // RTMP
  public void inputRtmpServerName(String value) throws KiteInteractionException {
    click(getElement(PageElements.RTMP_SERVER_NAME));
    if (value.contains(":")) {
      List<String> parts = Arrays.asList(value.split(":"));
      WebElement element = getElement(PageElements.RTMP_SERVER_NAME);
      element.clear();
      for (int index = 0; index < parts.size(); index++) {
        element.sendKeys(parts.get(index));
        if (index < parts.size() - 1) {
          element.sendKeys(Keys.SHIFT, Keys.SEMICOLON, Keys.SHIFT);
        }
      }
    } else {
      sendKeys(getElement(PageElements.RTMP_SERVER_NAME), value);
    }
  }

  public void inputRtmpStreamKey(String value) throws KiteInteractionException {
    click(getElement(PageElements.RTMP_PUBLISHING_KEY));
    getElement(PageElements.RTMP_PUBLISHING_KEY).clear();
    sendKeys(getElement(PageElements.RTMP_PUBLISHING_KEY), value);
  }

  // MILLICAST
  public void inputStreamName(String value) throws KiteInteractionException {
    logger.info("inputStreamName -> " + value);
    click(getElement(PageElements.STREAM_NAME));
    sendKeys(getElement(PageElements.STREAM_NAME), value);
  }

  public void inputPublishingToken(String value) throws KiteInteractionException {
    logger.info("inputPublishingToken -> " + value);
    click(getElement(PageElements.STREAM_PUBLISHING_TOKEN));
    getElement(PageElements.STREAM_PUBLISHING_TOKEN).clear();
    sendKeys(getElement(PageElements.STREAM_PUBLISHING_TOKEN), value);
  }

  public void pickCodec(String codec) throws KiteInteractionException {
    // assuming the stream setting is open
    click(getElement(PageElements.STREAM_CODEC_DROPDOWN));
    List<WebElement> streamTypes;
    logger.info("Choosing codec "  + codec);
    if (this.os.equals("WINDOWS")) {
      streamTypes = webDriver
          .findElements(By.xpath(this.elements.get(PageElements.STREAM_CODEC)));
      for (WebElement type : streamTypes) {
        logger.info("Found "  + type.getAttribute("Name"));
        if (type.getAttribute("Name").startsWith(codec)) {
          click(type);
          return;
        }
      }
    } else {
      streamTypes = this.getElementByXpath(this.elements.get(PageElements.STREAM_CODEC));
      for (WebElement type : streamTypes) {
        logger.info("Found "  + type.getAttribute("Name"));
        if (type.getTagName().startsWith(codec)) {
          click(type);
          return;
        }
      }
    }
    throw new KiteInteractionException("Could not find streaming service " + codec);
//    switch (codec.toLowerCase()) {
//      case "h264": {
//        click(webDriver.findElement(By.xpath(
//            elements.get(PageElements.STREAM_CODEC) + (os.equals("WINDOWS") ? "[1]" : "[0]"))));
//        break;
//      }
//      case "vp8": {
//        click(webDriver.findElement(By.xpath(
//            elements.get(PageElements.STREAM_CODEC) + (os.equals("WINDOWS") ? "[2]" : "[1]"))));
//        break;
//      }
//      case "vp9": {
//        click(webDriver.findElement(By.xpath(
//            elements.get(PageElements.STREAM_CODEC) + (os.equals("WINDOWS") ? "[3]" : "[2]"))));
//        break;
//      }
//      default: {
//        throw new KiteInteractionException(
//            "Currently we support only 3 codecs (1 -> h264; 2 -> vp8; 3 -> vp9)");
//      }
//    }
  }

  public void chooseStreamService(String serviceName) throws KiteInteractionException {
    click(getElement(PageElements.STREAM_TYPE_DROPDOWN));
    List<WebElement> streamTypes;
    logger.info("Choosing streaming service "  + serviceName);
    if (this.os.equals("WINDOWS")) {
      streamTypes = webDriver
          .findElements(By.xpath(this.elements.get(PageElements.STREAM_TYPE_OPTION)));
      for (WebElement type : streamTypes) {
        logger.info("Found "  + type.getAttribute("Name"));
        if (type.getAttribute("Name").startsWith(serviceName)) {
          click(type);
          return;
        }
      }
    } else {
      streamTypes = this.getElementByXpath(this.elements.get(PageElements.STREAM_TYPE_OPTION));
      for (WebElement type : streamTypes) {
        if (type.getTagName().startsWith(serviceName)) {
          click(type);
          return;
        }
      }
    }
    throw new KiteInteractionException("Could not find streaming service " + serviceName);
  }

  public List<String> getSteamType() throws KiteInteractionException {
    // click(getElement(PageElements.STREAM_TYPE_DROPDOWN));
    List<String> res = new ArrayList<>();
    List<WebElement> streamTypes;
    // waitAround(ONE_SECOND_INTERVAL);
    if (this.os.equals("WINDOWS")) {
      streamTypes = webDriver
          .findElements(By.xpath(this.elements.get(PageElements.STREAM_TYPE_OPTION)));
      for (WebElement type : streamTypes) {
        res.add(type.getAttribute("Name"));
      }
    } else {
      streamTypes = this.getElementByXpath(this.elements.get(PageElements.STREAM_TYPE_OPTION));
      for (WebElement type : streamTypes) {
        res.add(type.getTagName());
      }
    }

    return res;
  }

  @Override
  protected String getSettingOptionName() {
    return PageElements.STREAM_OPT;
  }

  @Override
  protected void addWindowsElements() {

    this.elements.put(PageElements.STREAM_TYPE_DROPDOWN, streamTypeOptionWIN);
    this.elements.put(PageElements.STREAM_TYPE_OPTION, streamTypeOptionItemWIN);

    this.elements.put(PageElements.RTMP_SERVER_NAME, rtmpServerNameTextFieldWIN);
    this.elements.put(PageElements.RTMP_PUBLISHING_KEY, rtmpStreamKeyTextFieldWIN);

    this.elements.put(PageElements.STREAM_NAME, streamNameTextEditWIN);
    this.elements.put(PageElements.STREAM_PUBLISHING_TOKEN, publishingTokenTextEditWIN);

    this.elements.put(PageElements.STREAM_CODEC_DROPDOWN, codecWIN);
    this.elements.put(PageElements.STREAM_CODEC, codecListItemWIN);
  }

  @Override
  protected void addMacElements() {

    this.elements.put(PageElements.STREAM_TYPE_DROPDOWN, streamTypeOptionMAC);
    this.elements.put(PageElements.STREAM_TYPE_OPTION, streamTypeOptionItemMAC);

    this.elements.put(PageElements.RTMP_SERVER_NAME, rtmpServerNameTextFieldMAC);
    this.elements.put(PageElements.RTMP_PUBLISHING_KEY, rtmpStreamKeyTextFieldMAC);

    this.elements.put(PageElements.STREAM_NAME, streamNameTextEditMAC);
    this.elements.put(PageElements.STREAM_PUBLISHING_TOKEN, publishingTokenTextEditMAC);

    this.elements.put(PageElements.STREAM_CODEC_DROPDOWN, codecMAC);
    this.elements.put(PageElements.STREAM_CODEC, codecListItemMAC);
  }

  @Override
  protected void addLinuxElements() {
    // todo
  }

}
