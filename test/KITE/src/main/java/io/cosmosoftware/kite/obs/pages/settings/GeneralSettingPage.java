package io.cosmosoftware.kite.obs.pages.settings;

import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

public class GeneralSettingPage extends SettingPage {

  // WINDOWS ------------------------------------------------------------------------------------------------
  private static String generalGroupElementWIN = "/Window/Window[1]/Custom/Group/Group/Group/Group/Group/Group";
  // MAC ------------------------------------------------------------------------------------------------
  private static String generalGroupElementMAC = "/AXApplication/AXWindow[0]/AXGroup/AXGroup";

  public GeneralSettingPage(Runner runner) {
    super(runner);
  }


  public List<String> getOutputOptions() {
    List<String> res = new ArrayList<>();
    if (this.os.equals("WINDOWS")) {
      List<WebElement> options = this.webDriver
          .findElements(
              By.xpath(elements.get(PageElements.GENERAL_GROUP_ELEMENT) + "[2]" + "/CheckBox"));
      for (WebElement option : options) {
        res.add(option.getAttribute("Name"));
      }
    } else {
      List<WebElement> options = this.getElementByXpath(
          elements.get(PageElements.GENERAL_GROUP_ELEMENT) + "[1]" + "/AXCheckBox");
      for (WebElement option : options) {
        res.add(option.getTagName());
      }
    }
    return res;
  }

  @Override
  protected String getSettingOptionName() {
    return PageElements.GENERAL_OPT;
  }

  @Override
  protected void addWindowsElements() {
    this.elements.put(PageElements.GENERAL_GROUP_ELEMENT, generalGroupElementWIN);
  }

  @Override
  protected void addMacElements() {
    this.elements.put(PageElements.GENERAL_GROUP_ELEMENT, generalGroupElementMAC);
  }

  @Override
  protected void addLinuxElements() {

  }

}
