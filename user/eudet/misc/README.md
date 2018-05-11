# Testing euCliMerger

* Purpose: Merge slow detector with fast detector
* Event definition: by slowest device 
* Synchronistation: by Trigger ID

## Using 

* `euCliMerger -i1 run_000044_ni.raw -i2 run_000044_tlu.raw -o 44merged.raw` up to now i1 must be the slower device

* for example `euCliReader -i 44merged.raw -e 0 -E 1`


```
<Event>
  <Type>2149999981</Type>
  <Extendword>1861316860</Extendword>
  <Description>MimosaTlu</Description>
  <Flag>0x00000010</Flag>
  <RunN>0</RunN>
  <StreamN>0</StreamN>
  <EventN>1</EventN>
  <TriggerN>14</TriggerN>
  <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
  <Timestamp>0  ->  0</Timestamp>
  <Block_Size>0</Block_Size>
  <SubEvents>
    <Size>24</Size>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>928560615</StreamN>
      <EventN>1</EventN>
      <TriggerN>14</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>1151355482</Extendword>
          <Description>NiRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>1393071404</StreamN>
          <EventN>1</EventN>
          <TriggerN>14</TriggerN>
          <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
          <Timestamp>0  ->  0</Timestamp>
          <Block_Size>2</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>13</EventN>
      <TriggerN>14</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>13</EventN>
          <TriggerN>14</TriggerN>
          <Timestamp>0x000000000087a5c8  ->  0x000000000087a5e1</Timestamp>
          <Timestamp>8889800  ->  8889825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>14</EventN>
      <TriggerN>15</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>14</EventN>
          <TriggerN>15</TriggerN>
          <Timestamp>0x000000000087ccd8  ->  0x000000000087ccf1</Timestamp>
          <Timestamp>8899800  ->  8899825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>15</EventN>
      <TriggerN>16</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>15</EventN>
          <TriggerN>16</TriggerN>
          <Timestamp>0x000000000087f3e8  ->  0x000000000087f401</Timestamp>
          <Timestamp>8909800  ->  8909825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>16</EventN>
      <TriggerN>17</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>16</EventN>
          <TriggerN>17</TriggerN>
          <Timestamp>0x0000000000881af8  ->  0x0000000000881b11</Timestamp>
          <Timestamp>8919800  ->  8919825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>17</EventN>
      <TriggerN>18</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>17</EventN>
          <TriggerN>18</TriggerN>
          <Timestamp>0x0000000000884208  ->  0x0000000000884221</Timestamp>
          <Timestamp>8929800  ->  8929825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>18</EventN>
      <TriggerN>19</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>18</EventN>
          <TriggerN>19</TriggerN>
          <Timestamp>0x0000000000886918  ->  0x0000000000886931</Timestamp>
          <Timestamp>8939800  ->  8939825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>19</EventN>
      <TriggerN>20</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>19</EventN>
          <TriggerN>20</TriggerN>
          <Timestamp>0x0000000000889028  ->  0x0000000000889041</Timestamp>
          <Timestamp>8949800  ->  8949825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>20</EventN>
      <TriggerN>21</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>20</EventN>
          <TriggerN>21</TriggerN>
          <Timestamp>0x000000000088b738  ->  0x000000000088b751</Timestamp>
          <Timestamp>8959800  ->  8959825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>21</EventN>
      <TriggerN>22</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>21</EventN>
          <TriggerN>22</TriggerN>
          <Timestamp>0x000000000088de48  ->  0x000000000088de61</Timestamp>
          <Timestamp>8969800  ->  8969825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>22</EventN>
      <TriggerN>23</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>22</EventN>
          <TriggerN>23</TriggerN>
          <Timestamp>0x0000000000890558  ->  0x0000000000890571</Timestamp>
          <Timestamp>8979800  ->  8979825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>23</EventN>
      <TriggerN>24</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>23</EventN>
          <TriggerN>24</TriggerN>
          <Timestamp>0x0000000000892c68  ->  0x0000000000892c81</Timestamp>
          <Timestamp>8989800  ->  8989825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>24</EventN>
      <TriggerN>25</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>24</EventN>
          <TriggerN>25</TriggerN>
          <Timestamp>0x0000000000895378  ->  0x0000000000895391</Timestamp>
          <Timestamp>8999800  ->  8999825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>25</EventN>
      <TriggerN>26</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>25</EventN>
          <TriggerN>26</TriggerN>
          <Timestamp>0x0000000000897a88  ->  0x0000000000897aa1</Timestamp>
          <Timestamp>9009800  ->  9009825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>26</EventN>
      <TriggerN>27</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>26</EventN>
          <TriggerN>27</TriggerN>
          <Timestamp>0x000000000089a198  ->  0x000000000089a1b1</Timestamp>
          <Timestamp>9019800  ->  9019825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>27</EventN>
      <TriggerN>28</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>27</EventN>
          <TriggerN>28</TriggerN>
          <Timestamp>0x000000000089c8a8  ->  0x000000000089c8c1</Timestamp>
          <Timestamp>9029800  ->  9029825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>28</EventN>
      <TriggerN>29</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>28</EventN>
          <TriggerN>29</TriggerN>
          <Timestamp>0x000000000089efb8  ->  0x000000000089efd1</Timestamp>
          <Timestamp>9039800  ->  9039825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>29</EventN>
      <TriggerN>30</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>29</EventN>
          <TriggerN>30</TriggerN>
          <Timestamp>0x00000000008a16c8  ->  0x00000000008a16e1</Timestamp>
          <Timestamp>9049800  ->  9049825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>30</EventN>
      <TriggerN>31</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>30</EventN>
          <TriggerN>31</TriggerN>
          <Timestamp>0x00000000008a3dd8  ->  0x00000000008a3df1</Timestamp>
          <Timestamp>9059800  ->  9059825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>31</EventN>
      <TriggerN>32</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>31</EventN>
          <TriggerN>32</TriggerN>
          <Timestamp>0x00000000008a64e8  ->  0x00000000008a6501</Timestamp>
          <Timestamp>9069800  ->  9069825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>32</EventN>
      <TriggerN>33</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>32</EventN>
          <TriggerN>33</TriggerN>
          <Timestamp>0x00000000008a8bf8  ->  0x00000000008a8c11</Timestamp>
          <Timestamp>9079800  ->  9079825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>33</EventN>
      <TriggerN>34</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>33</EventN>
          <TriggerN>34</TriggerN>
          <Timestamp>0x00000000008ab308  ->  0x00000000008ab321</Timestamp>
          <Timestamp>9089800  ->  9089825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>34</EventN>
      <TriggerN>35</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>34</EventN>
          <TriggerN>35</TriggerN>
          <Timestamp>0x00000000008ada18  ->  0x00000000008ada31</Timestamp>
          <Timestamp>9099800  ->  9099825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
    <Event>
      <Type>2149999981</Type>
      <Extendword>171577627</Extendword>
      <Description>Ex0Tg</Description>
      <Flag>0x00000018</Flag>
      <RunN>44</RunN>
      <StreamN>1433206861</StreamN>
      <EventN>35</EventN>
      <TriggerN>36</TriggerN>
      <Timestamp>0x0000000000000000  ->  0x0000000000000000</Timestamp>
      <Timestamp>0  ->  0</Timestamp>
      <Block_Size>0</Block_Size>
      <SubEvents>
        <Size>1</Size>
        <Event>
          <Type>2149999981</Type>
          <Extendword>3634980144</Extendword>
          <Description>TluRawDataEvent</Description>
          <Flag>0x00000010</Flag>
          <RunN>44</RunN>
          <StreamN>4008428646</StreamN>
          <EventN>35</EventN>
          <TriggerN>36</TriggerN>
          <Timestamp>0x00000000008b0128  ->  0x00000000008b0141</Timestamp>
          <Timestamp>9109800  ->  9109825</Timestamp>
          <Tags>
            <Tag>trigger=������</Tag>
          </Tags>
          <Block_Size>0</Block_Size>
        </Event>
      </SubEvents>
    </Event>
  </SubEvents>
</Event>
There are 37545Events
```
