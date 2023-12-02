<a id="anchor"></a>
<h1 align="center"> Brain_Ring </h1>

### Проект игровой комнаты. 
вопросы можно задать автору проета в [телеграм](https://t.me/nicelightio)

## ABOUT 
Проект представляет себя комнату с потолком, на котором выложен 2D mapping из ленты ws2812 по проекту [WLED](https://kno.wled.ge/) разделенный на две половины. на полу каждой половины стоят столбы с кнопками для двух команд. Столб подсвечен лентой ws2812 по проекту [GyverPanelWifi](https://github.com/vvip-68/GyverPanelWiFi/tree/master), у последнего есть крупная ветка [на форуме](https://community.alexgyver.ru/threads/wifi-lampa-budilnik-obsuzhdenie-proshivki-ot-gunner47.2418/).
 Оператор комнаты управляет музыкой, светом и игрой из веб странички. Игра по сути это брейнринг, две команды, задается вопрос, кто первый шлепнул по кнопке, у того кнопка засветилась, потолок засветился особо, и команда отвечает на вопрос. ответила правильно - свет победный, потом свет перебивки и заново состояние ожидания нажатия на нопки одной из команд. 
В веб интерфейсе оператора так же доступна светомузыка на потолке и кнопках.
По средние комнаты расположено место с футпринтом ладони из акрила, он тоже немного подсвечен.  Если к нему приложить руку, весь свет плавно тухнет в комнате. При удержании руки более 5 секунд, рука становится яркой, общий свет в комнате тухнет до нуля, резко включается и (через установленное время задержки) происходит фотография комнаты камерой, расположенной рядом с футпринтом руки. Если руку недодержали 5 секунд, подсветка восстанавливается, рука тухнет.
Если активирован вариант игры, то:
- запускается эффект перебивки на 5 секунд
- левая кнопка подсвечена переливающимся синим, правая красным ( тут не плох noise effect )
- кто первый нажимает на кнопку, у того яркость становится максимум на 20 милисекунд, потом обратно тухнет, после этого плавно за 5 секунд яркость нарастает до максимума. 
- теперь оператор выбирает кто победил ( левый, правый, никто) и у победившего запускается  яркий эффект (не плох cyclone эффект) на 5 секунд, после кнопка становится обратно своим цветом noise и яркость за 3 секунды тухнет на минимум
- запускается эффект перебивки на 5 секунд
- - процесс игры повторяется заново
В случае правильного ответа на вопрос, на потолкеотрисовывается веселая подсветка.

## DOCUMENTATION 
Топология. В комнате четыре  esp32, подключены к роутеру по wifi, как клиенты. 
1. esp32 192.168.3.201 - web server с веб мордой , из которой можно отправлять сигналы на потолок, на кнопки а так же делать фотографии. Расположена по центру комнаты, подключена к акриловой руке. 
2. esp32 192.168.3.202 - подсветка потолка ,  отрабатывает стандартный проект wled, получает http команды от .201
3. esp32 192.168.3.203 - Left - левая кнопка с постветкой, основана на форке проекта [GyverLamp](https://alexgyver.ru/gyverlamp/) который называется [GyverPanelWifi](https://github.com/vvip-68/GyverPanelWiFi/tree/master) получает UDP команды от .201
4. esp32 192.168.3.204 - Right- правая кнопка, по аналогии с левой.


[API кнопок, основанных на GyverPanel](https://github.com/vvip-68/GyverPanelWiFi/wiki/API-%D1%83%D0%BF%D1%80%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D1%8F-%D1%83%D1%81%D1%82%D1%80%D0%BE%D0%B9%D1%81%D1%82%D0%B2%D0%BE%D0%BC) 
Сервер отправляет UDP пакет кнопкам. Пакет  начинается $ заканчивается ;
Примеры пакетов:   
* $16 0;     - ручной режим смены эффектов ( всегда активируем); 
* $6 7|BR|EF|SS|SE запрос параметров от устройства
        BR - яроксть
        EF - id эффекта
        SS - параметр \#1 эффекта
        SE - скорость эффекта
* $4 0 D; установить текущий уровень общей яркости  
        D - яркость в диапазоне 1..255
*  $8 0 N;     - включить эффект N
* $8 1 N D;   - D -> параметр \#1 для эффекта N;
* $8 6 N D;   - D -> контрастность эффекта N;
* $8 8 N D;   - D -> скорость эффекта N;
* $14 0;      - Черный экран (выкл);  
* $14 1;      - Белый экран (освещение);  
* $14 2;      - Цветной экран;  
* $14 3;      - Огонь;  
* $14 4;      - Конфетти;  
* $14 5;      - Радуга;  
* $14 6;      - Матрица;  
* $14 7;      - Светлячки;  
*
API потолка, основанного на WLED [примтивный GET](https://kno.wled.ge/interfaces/http-api/) или [расширенный POST json](https://kno.wled.ge/interfaces/json-api/)
Для простых переключений формируем GET запрос , к примеру 
192.168.3.202/win&A=255
еще варианты:
* &A= 	0 to 255 	Master brightness
* &A=~30 увеличить яркость на 30
* &A=~-20 уменьшить яркость на 20
* &T= 	0, 1, or 2 	Master Off/On/Toggle
* &PL= 1 установить пресет \#1
* &P1=1&P2=3&PL=~ каждый очередной вызов будет переключаться на следующий эффект из списка Р1-Р2 ( для дискотеки можно)
* &FX= 	0 to 101 	LED Effect Index
* &SX= 	0 to 255 	Effect Speed
* &IX= 	0 to 255 	Effect Intensity
* &ND 	 Toggles nightlight on but uses default duration

## INSTALLATION

1. Подключение лент 
![Подключение лент](/left_right_buttons/GyverPanelWiFi-master/schemes/scheme1.jpg)

Настройка WLED 
заходим в браузере на 192.168.3.202 
Вкладка Segments - Segment 0 - переименовываем в LEFT - в раскрывающемся списке выбираем левую часть потолка по координатам x, y, сохраняем. 
ниже Add Segment - его так же переименовываем в RIGHT - в раскрывающемся списке выбираем правую часть потолка, сохраняем.
Создаем пресеты. 
Делаем так чтобы иконки "часики" и  галочки были над LEFT и RIGHT сегментами
вкладка Colors выбираем во втором ползунке температуру белого цвета, комфортную для помещения
вкладка Effects - Candle Multi - настраиваем ползунками так чтобы весь потолок светился достаточно ярко для проведения игры, и эффект мерцания свечей был не сильно раздражающим
вкладка Presets - выбираем '+ Preset' - название: White Light - quick load label: O первых три галочки стоят, четвертую не надо - saveto ID: 1 -- Save
Здесь важно какой ID мы будем задавать. игра будет отрабатывать по этим айдишникам. Сейчас мы создали общее освещение во время игры.



[Вверх](#anchor)
