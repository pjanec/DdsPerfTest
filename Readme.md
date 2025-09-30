# Dds Testing Tool

Features
--------
 * Creating given number of publishers and (potentially different) number subscriber across multiple machines.
    - You can select on what machines to allow creating publishers/subscriber
    - **Scalable UI**: Automatically adapts to 60+ app instances
      - **Adaptive table height**: Grows from 150px to 500px based on number of apps
      - **Automatic scrolling**: Seamlessly handles large deployments with scroll support
      - **Visual indicators**: Shows app count and scroll hints for large deployments
    - If total number request is greater than the number of machines
      - They will be evenly distributed on machines
      - More may get created per machine; each using different Participant to simulate multiple apps per machine.

 * Multiple different message classes can be simulated at once
    - You can define them in MsgDefs.csv with different
      - QoS settings 
      - Read strategies
         - Poll ... periodic polling using dds_take
         - ListenImmed ... upon listener callback all available messaged taken using dds_take
         - ListenDefer ... listener just sets a flag; messages then polled from main loop if the flag is set
      - **DDS Partitions** ... specify which partition(s) publishers and subscribers operate in
         - Empty partition name uses the default (global) partition
         - Single partition: "Robot1", "Robot2", etc.
         - Multiple partitions: "Robot1,Robot2,SharedData" (comma-separated)
         - Enables realistic testing of partitioned DDS systems with proper data isolation
    - You can interactively change 
      - Disabled ... create/destroy publishers & subscriber
           - Click repeatedly to simulate frequent changes
      - Rate ... num msgs per second sent by each publisher
           - 0 = publisher created but no msgs sent; use to temporarily stop sending msgs while keeping subscribers alive
           - note: the resulting rate is usually slover because of 4 msec resolution of the system tick
      - Size ... rough size in bytes of each message
      - Number of publishers and subscribers per app:
           - **Smart table interface** with App | Computer:PID | Pubs | Subs columns
           - **Auto-sizing**: Table height adapts automatically (150px - 500px)
           - **Scroll support**: Smoothly handles 60+ apps with frozen header row
           - **Total display**: Shows counts in collapsible header: "Publishers & Subscribers (Pubs: X, Subs: Y)"
           - **Flexible batch operations**:
             - Input fields to specify custom pub/sub counts (any number)
             - "Set All Pubs/Subs" buttons use the entered value
             - "Clear All Pubs/Subs" buttons set everything to 0
             - Separate controls for publishers and subscribers
           - **Auto-select**: Input fields auto-select text for quick editing
           - **Stable UI**: Header stays open when totals change (fixed IDs)

 * Central monitoring of the subscriber stats from all stations; per each publisher
      - Rate ... approx number of messages received
      - Received ... number of messages recieved (from publishers of same msg class)
      - Lost ... number of messages missed
         - Note: loss may be reported if we keep the subscriber alive while we remove & re-add a publisher, not a bug
      - **Partition** ... shows which partition(s) each subscriber is listening to

 * **Partition Isolation Testing**
      - Verify that subscribers only receive data from publishers in matching partitions
      - Test scenarios with multiple partitioned systems operating independently
      - Monitor partition information in real-time through the UI

 * Reset dds infrastructure

Notes
-----
 * Each app gets own unique integer id shown in the title (happens after selecting the master station).
 * Each publisher/subscriper in the app gets its unique integer id (0..max-1)
 * Lost message detection based on simple msg sequence number (per publisher)
 * App ticks at 60Hz by default (can be increased to 10kHz for ultra high prec timing - using spinwait)
 * **Partition isolation is enforced at the DDS level** - publishers and subscribers in different partitions cannot communicate
 * **UI automatically scales** efficiently from small (few apps) to large deployments (60+ apps)
 * **Table grows dynamically** - starts at ~10 visible rows, expands as needed up to ~20 rows with scrolling
 * **Collapsible sections remain stable** - headers don't close when totals change


How to use
-----------
 1. Run on multiple PCs.
 2. Collect apps.
 3. Select message classes to use (including partitioned message types)
 4. Enable the settings
 5. For each message class play with the settings
    - Note: Partition information is displayed read-only in the message settings
    - Click on "Publishers & Subscribers" header to expand the table
    - **For batch operations**:
      - Enter desired count in the input field next to "Set All Pubs/Subs"
      - Click "Set All Pubs/Subs" to apply that number to all available apps
      - Use "Clear All Pubs/Subs" to quickly set everything to 0
    - **For 60+ apps**: Table will show app count and provide smooth scrolling
    - Edit individual app pub/sub counts directly in the table (auto-select for quick editing)
 6. In the SubscrStats window, watch the Lost number and **Partition column** to verify isolation
