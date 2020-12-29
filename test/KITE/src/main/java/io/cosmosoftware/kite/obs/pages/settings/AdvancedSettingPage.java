package io.cosmosoftware.kite.obs.pages.settings;

import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

public class AdvancedSettingPage extends SettingPage {

  // WINDOWS ------------------------------------------------------------------------------------------------
  private static String advancedGroupElementWIN = "/Window/Window[1]/Custom/Group/Group/Group/Group/Group/Group";
  // MAC ------------------------------------------------------------------------------------------------
  private static String advancedGroupElementMAC = "/AXApplication/AXWindow[0]/AXGroup[0]/AXGroup";

  public AdvancedSettingPage(Runner runner) {
    super(runner);
  }

  @Override
  protected String getSettingOptionName() {
    return PageElements.ADVANCED_OPT;
  }

  @Override
  protected void addWindowsElements() {
    this.elements.put(PageElements.ADVANCED_GROUP_ELEMENT, advancedGroupElementWIN);
  }

  @Override
  protected void addMacElements() {
    this.elements.put(PageElements.ADVANCED_GROUP_ELEMENT, advancedGroupElementMAC);
  }

  @Override
  protected void addLinuxElements() {

  }

  public List<String> getSettingGroupLabels() {
    List<String> res = new ArrayList<>();
    if (this.os.equals("WINDOWS")) {
      List<WebElement> groups = this.webDriver
          .findElements(By.xpath(elements.get(PageElements.ADVANCED_GROUP_ELEMENT)));
      for (WebElement group : groups) {
        res.add(group.getAttribute("Name"));
      }
    } else {
      List<WebElement> groups = this
          .getElementByXpath(elements.get(PageElements.ADVANCED_GROUP_ELEMENT));
      for (WebElement group : groups) {
        res.add(group.getTagName());
      }
    }
    return res;
  }
}
