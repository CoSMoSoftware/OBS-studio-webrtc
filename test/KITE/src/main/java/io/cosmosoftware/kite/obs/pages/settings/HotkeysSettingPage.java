package io.cosmosoftware.kite.obs.pages.settings;

import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

public class HotkeysSettingPage extends SettingPage {

  // WINDOWS ------------------------------------------------------------------------------------------------
  private static String hotkeysOptionWIN = "/Window/Window[1]/Custom/Group/Group/Group/Text";
  // MAC ------------------------------------------------------------------------------------------------
  private static String hotkeysOptionMAC = "/AXApplication/AXWindow[0]/AXGroup/AXStaticText";

  public HotkeysSettingPage(Runner runner) {
    super(runner);
  }

  @Override
  protected String getSettingOptionName() {
    return PageElements.HOTKEY_OPT;
  }

  @Override
  protected void addWindowsElements() {
    this.elements.put(PageElements.HOTKEY_OPTION, hotkeysOptionWIN);
  }

  @Override
  protected void addMacElements() {
    this.elements.put(PageElements.HOTKEY_OPTION, hotkeysOptionMAC);
  }

  @Override
  protected void addLinuxElements() {

  }

  public List<String> getHotKeyOptions() {
    List<String> res = new ArrayList<>();
    if (this.os.equals("WINDOWS")) {
      List<WebElement> options = this.webDriver
          .findElements(By.xpath(elements.get(PageElements.HOTKEY_OPTION)));
      for (WebElement option : options) {
        res.add(option.getText());
      }
    } else {
      List<WebElement> options = this.getElementByXpath(elements.get(PageElements.HOTKEY_OPTION));
      for (WebElement option : options) {
        res.add(option.getText());
      }
    }
    return res;
  }
}
