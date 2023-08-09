# Dds Testing Tool

Features
--------
 * Creating given number of publishers and (potentially different) number subscriber across multiple machines.
    - You can select on what machines to allow creating publishers/subscriber
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
    - You can interactively change 
      - Disabled ... create/destroy publishers & subscriber
           - Click repeatedly to simulate frequent changes
      - Rate ... num msgs per second sent by each publisher
           - 0 = publisher created but no msgs sent; use to temporarily stop sending msgs while keeping subscribers alive
           - note: the resulting rate is usually slover because of 4 msec resolution of the system tick
      - Size ... rough size in bytes of each message
      - Number of publishers (total across all stations)
           - click on the app mask to create/remove publishers for given app
      - Number of subscribers (total across all stations)

 * Central monitoring of the subscriber stats from all stations; per each publisher
      - Rate ... approx number of messages received
      - Received ... number of messages recieved (from publishers of same msg class)
      - Lost ... number of messages missed
         - Note: loss may be reported if we keep the subscriber alive while we remove & re-add a publisher, not a bug

 * Reset dds infrastructure

Notes
-----
 * Each app gets own unique integer id shown in the title (happens after selecting the master station).
 * Each publisher/subscriper in the app gets its unique integer id (0..max-1)
 * Lost message detection based on simple msg sequence number (per publisher)
 * App ticks at 60Hz by default (can be increased to 10kHz for ultra high prec timing - using spinwait)


How to use
-----------
 1. Run on multiple PCs.
 2. Collect apps.
 3. Select message classes to use
 4. Enable the settings
 5. For each message class play with the settings
 6. In the SubscrStats window, watch the Lost number
