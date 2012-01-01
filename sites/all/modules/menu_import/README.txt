$Id

WHAT IS THIS MODULE FOR?
You may need it when you want to:

1. Create a multilevel menu, without specific details about where the menu links
  will point to. You may need this when you want to create a prototype of your future site,
  or when you haven't decided yet where each menu link will point to. The module will
  create the menu for you using optionally provided external or local Drupal URLs.
  See the import file structure for details.

2. Create a multilevel menu with some/all links pointing to some stub content.
  You may need this when you want to create a prototype of your future site,
  and you haven't decided yet about the content but you need some tangible content
  to be available.

3. Update an existing menu by adding new items. You may add new items the same way as described
  in points 1 and 2 above.


USAGE INSTRUCTIONS:

1. Install the module and configure permissions to allow import.

2. Prepare a site map file

  Menu structure must follow this example:

    Page1
    - Page2
    - Page3
    Page4
    - Page5
    -- Page6

  or this

  Page1
  * Page2
  ** Page3
  Page4
  * Page5
  ** Page6
  *** Page7

  You may optionally specify path alias (alternative path)
  or external URL. Writing it after node's title, separating by semicolon or vertical bar:
  External URLs should ALWAYS start with "http://". Also, you can provide
  a description for menu item(s), by putting it in the third "column".

    Page1|node/1|This is an optional description
    - Page2
    - Page3||The line above and this one will point to <front>
    Page4;http://domain.com/;Visit domain.com!
    - Page5;http://mail.com/index.php
    -- Page6 ; non/existent/path ; will be replaced with <front>

  Space(s) between indentation symbol "*" or "-" and menu/node title are optional,
  however you cannot put spaces between indentation symbols like "* * *" or "-- - -".

  Use examples from "tests" directory for better understanding of the syntax.

3. Go to "Structure" -> "Menus"

4. Select "Menu import" tab.

5. Select the site map file created earlier and specify necessary options.

6. Submit and see the new menu structure with stub content created automatically.

7. Enjoy your saved time ;)
