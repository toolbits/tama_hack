/*
**      IridiumFrameworks
**
**      Original Copyright (C) 2012 - 2012 HORIGUCHI Junshi.
**                                          http://iridium.jp/
**                                          zap00365@nifty.com
**      Portions Copyright (C) <year> <author>
**                                          <website>
**                                          <e-mail>
**      Version     C++03
**      Website     http://iridium.jp/
**      E-mail      zap00365@nifty.com
**
**      This source code is for Xcode.
**      Xcode 5.1.1 (Apple LLVM 5.1)
**
**      IRXTime.h
**
**      ------------------------------------------------------------------------
**
**      The MIT License (MIT)
**
**      Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
**      associated documentation files (the "Software"), to deal in the Software without restriction,
**      including without limitation the rights to use, copy, modify, merge, publish, distribute,
**      sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
**      furnished to do so, subject to the following conditions:
**      The above copyright notice and this permission notice shall be included in all copies or
**      substantial portions of the Software.
**      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
**      BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
**      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
**      WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
**      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
**      以下に定める条件に従い、本ソフトウェアおよび関連文書のファイル（以下「ソフトウェア」）の複製を
**      取得するすべての人に対し、ソフトウェアを無制限に扱うことを無償で許可します。
**      これには、ソフトウェアの複製を使用、複写、変更、結合、掲載、頒布、サブライセンス、および、または販売する権利、
**      およびソフトウェアを提供する相手に同じことを許可する権利も無制限に含まれます。
**      上記の著作権表示および本許諾表示を、ソフトウェアのすべての複製または重要な部分に記載するものとします。
**      ソフトウェアは「現状のまま」で、明示であるか暗黙であるかを問わず、何らの保証もなく提供されます。
**      ここでいう保証とは、商品性、特定の目的への適合性、および権利非侵害についての保証も含みますが、それに限定されるものではありません。
**      作者または著作権者は、契約行為、不法行為、またはそれ以外であろうと、ソフトウェアに起因または関連し、
**      あるいはソフトウェアの使用またはその他の扱いによって生じる一切の請求、損害、その他の義務について何らの責任も負わないものとします。
*/

#ifndef __IR_IRXTIME_H
#define __IR_IRXTIME_H

#include "IRXTimeDiff.h"

namespace ir {

//! 時刻を表現するクラス
/*!
    時刻を扱うには IRXTime クラスを利用します。
    ２つの時刻の差を扱うには IRXTimeDiff クラスを利用します。
 
    IRXTime クラスの代表的な使い方
 
    使い方１：「年/月/日 (曜日) 時:分:秒」の書式に従って文字列を書き出す
    @code
    IRXTime time;
    string str;
 
    time = IRXTime::currentTime();
    str = time.format("%YYYY/%MM/%DD (%WEK) %hh:%mm:%ss");
    cout << str << endl;
    @endcode
    使い方２：「年/月/日 (曜日) 時:分:秒」の書式に従って文字列から読み込む
    @code
    string str;
    IRXTime time;
    IRXTime::ErrorEnum error;
 
    str = "2012/11/07 (WED) 14:48:45";
    error = time.parse("%YYYY/%MM/%DD (%WEK) %hh:%mm:%ss", str);
    if (error == IRXTime::ERROR_OK) {
        cout << "time = " << time.format("%YYYY/%MM/%DD (%WEK) %hh:%mm:%ss") << endl;
    }
    else {
        cout << "error = " << error << endl;
    }
    @endcode
    使い方３：柔軟な文字列の読み込み
    @code
    string str;
    IRXTime time;
    IRXTime::ErrorEnum error;
 
    str = "today  is 2012/11/7    14:48:45 WED";
    error = time.parse("%***** is %YYYY/%M/%D %hh:%mm:%ss", str);
    if (error == IRXTime::ERROR_OK) {
        cout << "time = " << time.formatYMD() << endl;
    }
    else {
        cout << "error = " << error << endl;
    }
    @endcode
    使い方４：時刻を表す文字列の変換
    @code
    string str;
    IRXTime time;
    IRXTime::ErrorEnum error;
 
    str = "NOV 7/2012 (WED) - 14:48:45";
    error = time.parse("%MTH %D/%Y (%***) - %h:%m:%s", str);
    if (error == IRXTime::ERROR_OK) {
        str = time.format("%YYYY/%MM/%DD (%WEEK) %hh:%mm:%ss");
        cout << "time = " << str << endl;
    }
    else {
        cout << "error = " << error << endl;
    }
    @endcode
 */
class IRXTime {
    public:
        /*!
            エラーを表す定数です。
         */
        enum ErrorEnum {
            //! エラーなし
            ERROR_OK,
            //! 一般的なエラー
            ERROR_FAILED,
            //! 書式が正しくない
            ERROR_INVALID_FORMAT,
            //! エラー定数の最大値
            ERROR_LIMIT
        };
        /*!
            曜日を表す定数です。
         */
        enum DayOfWeekEnum {
            //! 日曜日
            DAYOFWEEK_SUNDAY,
            //! 月曜日
            DAYOFWEEK_MONDAY,
            //! 火曜日
            DAYOFWEEK_TUESDAY,
            //! 水曜日
            DAYOFWEEK_WEDNESDAY,
            //! 木曜日
            DAYOFWEEK_THURSDAY,
            //! 金曜日
            DAYOFWEEK_FRIDAY,
            //! 土曜日
            DAYOFWEEK_SATURDAY,
            //! 曜日定数の最大値
            DAYOFWEEK_LIMIT
        };
    private:
        enum SearchEnum {
            SEARCH_PERCENT,
            SEARCH_MONTH,
            SEARCH_MTH,
            SEARCH_WEEK,
            SEARCH_WEK,
            SEARCH_YOUBI,
            SEARCH_AMPM,
            SEARCH_GOZENGOGO,
            SEARCH_LIMIT
        };
    
    private:
                int                     _year;
                int                     _month;
                int                     _day;
                int                     _hour;
                int                     _minute;
                int                     _second;
                int                     _days;
                DayOfWeekEnum           _week;
    
    public:
        /*!
            コンストラクタです。
            1970/01/01 00:00:00 が初期値となります。
         */
        explicit                        IRXTime                         (void);
        /*!
            コンストラクタです。
            引数に指定した値が初期値となります。
            @param [in] param 初期値
         */
                                        IRXTime                         (IRXTime const& param);
        /*!
            コンストラクタです。
            引数に指定した年・月・日・時・分・秒をもとにして初期値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 年の初期値
            @param [in] month 月の初期値
            @param [in] day 日の初期値
            @param [in] hour 時の初期値
            @param [in] minute 分の初期値
            @param [in] second 秒の初期値
         */
        explicit                        IRXTime                         (int year, int month, int day, int hour, int minute, int second);
        /*!
            コンストラクタです。
            引数に指定した年・日・時・分・秒をもとにして初期値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 年の初期値
            @param [in] day 日の初期値
            @param [in] hour 時の初期値
            @param [in] minute 分の初期値
            @param [in] second 秒の初期値
         */
        explicit                        IRXTime                         (int year, int day, int hour, int minute, int second);
        /*!
            コンストラクタです。
            引数に指定した年・時・分・秒をもとにして初期値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 年の初期値
            @param [in] hour 時の初期値
            @param [in] minute 分の初期値
            @param [in] second 秒の初期値
         */
        explicit                        IRXTime                         (int year, int hour, int minute, int second);
        /*!
            コンストラクタです。
            引数に指定した年・分・秒をもとにして初期値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 年の初期値
            @param [in] minute 分の初期値
            @param [in] second 秒の初期値
         */
        explicit                        IRXTime                         (int year, int minute, int second);
        /*!
            コンストラクタです。
            引数に指定した年・秒をもとにして初期値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 年の初期値
            @param [in] second 秒の初期値
         */
        explicit                        IRXTime                         (int year, int second);
        /*!
            コンストラクタです。
            引数に指定した POSIX 時刻をもとにして初期値を設定します。
            @param [in] param POSIX 時刻の初期値
         */
        explicit                        IRXTime                         (time_t param);
        /*!
            デストラクタです。
         */
                                        ~IRXTime                        (void);
        /*!
            値を代入します。
            @param [in] param 代入する値
            @return 自分への参照
         */
                IRXTime&                operator=                       (IRXTime const& param);
        /*!
            時刻差を加算します。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                operator+=                      (IRXTimeDiff const& param);
        /*!
            時刻差を減算します。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                operator-=                      (IRXTimeDiff const& param);
        /*!
            引数に指定した時刻の要素を取得します。
            @param [in] param 取得する要素（大文字と小文字は区別しない）
                @arg "year" : 年
                @arg "month" : 月
                @arg "day" : 日
                @arg "hour" : 時
                @arg "minute" : 分
                @arg "second" : 秒
            @return 現在の値
            @retval <0 指定した要素が存在しない
            @retval >=0 取得に成功
         */
                int                     operator[]                      (std::string const& param) const;
        /*!
            値を設定します。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                set                             (IRXTime const& param);
        /*!
            引数に指定した年・月・日・時・分・秒をもとにして値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 設定する年
            @param [in] month 設定する月
            @param [in] day 設定する日
            @param [in] hour 設定する時
            @param [in] minute 設定する分
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                set                             (int year, int month, int day, int hour, int minute, int second);
        /*!
            値を年・月・日・時・分・秒・曜日に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] year 現在の年
                @arg NULL : 年を返さない
                @arg その他
            @param [out] month 現在の月
                @arg NULL : 月を返さない
                @arg その他
            @param [out] day 現在の日
                @arg NULL : 日を返さない
                @arg その他
            @param [out] hour 現在の時
                @arg NULL : 時を返さない
                @arg その他
            @param [out] minute 現在の分
                @arg NULL : 分を返さない
                @arg その他
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @param [out] week 現在の曜日
                @arg NULL : 曜日を返さない
                @arg その他
            @return なし
         */
                void                    get                             (int* year, int* month, int* day, int* hour, int* minute, int* second, DayOfWeekEnum* week = NULL) const;
        /*!
            引数に指定した年・日・時・分・秒をもとにして値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 設定する年
            @param [in] day 設定する日
            @param [in] hour 設定する時
            @param [in] minute 設定する分
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                set                             (int year, int day, int hour, int minute, int second);
        /*!
            値を年・日・時・分・秒・曜日に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] year 現在の年
                @arg NULL : 年を返さない
                @arg その他
            @param [out] day 現在の日
                @arg NULL : 日を返さない
                @arg その他
            @param [out] hour 現在の時
                @arg NULL : 時を返さない
                @arg その他
            @param [out] minute 現在の分
                @arg NULL : 分を返さない
                @arg その他
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @param [out] week 現在の曜日
                @arg NULL : 曜日を返さない
                @arg その他
            @return なし
         */
                void                    get                             (int* year, int* day, int* hour, int* minute, int* second, DayOfWeekEnum* week = NULL) const;
        /*!
            引数に指定した年・時・分・秒をもとにして値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 設定する年
            @param [in] hour 設定する時
            @param [in] minute 設定する分
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                set                             (int year, int hour, int minute, int second);
        /*!
            値を年・時・分・秒に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] year 現在の年
                @arg NULL : 年を返さない
                @arg その他
            @param [out] hour 現在の時
                @arg NULL : 時を返さない
                @arg その他
            @param [out] minute 現在の分
                @arg NULL : 分を返さない
                @arg その他
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @return なし
         */
                void                    get                             (int* year, int* hour, int* minute, int* second) const;
        /*!
            引数に指定した年・分・秒をもとにして値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 設定する年
            @param [in] minute 設定する分
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                set                             (int year, int minute, int second);
        /*!
            値を年・分・秒に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] year 現在の年
                @arg NULL : 年を返さない
                @arg その他
            @param [out] minute 現在の分
                @arg NULL : 分を返さない
                @arg その他
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @return なし
         */
                void                    get                             (int* year, int* minute, int* second) const;
        /*!
            引数に指定した年・秒をもとにして値を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 設定する年
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                set                             (int year, int second);
        /*!
            値を年・秒に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] year 現在の年
                @arg NULL : 年を返さない
                @arg その他
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @return なし
         */
                void                    get                             (int* year, int* second) const;
        /*!
            引数に指定した POSIX 時刻をもとにして値を設定します。
            @param [in] param 設定する POSIX 時刻
            @return 自分への参照
         */
                IRXTime&                set                             (time_t param);
        /*!
            日付部分を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setDate                         (IRXTime const& param);
        /*!
            引数に指定した年・月・日をもとにして日付部分を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 設定する年
            @param [in] month 設定する月
            @param [in] day 設定する日
            @return 自分への参照
         */
                IRXTime&                setDate                         (int year, int month, int day);
        /*!
            日付部分を年・月・日・曜日に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] year 現在の年
                @arg NULL : 年を返さない
                @arg その他
            @param [out] month 現在の月
                @arg NULL : 月を返さない
                @arg その他
            @param [out] day 現在の日
                @arg NULL : 日を返さない
                @arg その他
            @param [out] week 現在の曜日
                @arg NULL : 曜日を返さない
                @arg その他
            @return なし
         */
                void                    getDate                         (int* year, int* month, int* day, DayOfWeekEnum* week = NULL) const;
        /*!
            引数に指定した年・日をもとにして日付部分を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] year 設定する年
            @param [in] day 設定する日
            @return 自分への参照
         */
                IRXTime&                setDate                         (int year, int day);
        /*!
            日付部分を年・日・曜日に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] year 現在の年
                @arg NULL : 年を返さない
                @arg その他
            @param [out] day 現在の日
                @arg NULL : 日を返さない
                @arg その他
            @param [out] week 現在の曜日
                @arg NULL : 曜日を返さない
                @arg その他
            @return なし
         */
                void                    getDate                         (int* year, int* day, DayOfWeekEnum* week = NULL) const;
        /*!
            時刻部分を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setTime                         (IRXTime const& param);
        /*!
            引数に指定した時・分・秒をもとにして時刻部分を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] hour 設定する時
            @param [in] minute 設定する分
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                setTime                         (int hour, int minute, int second);
        /*!
            時刻部分を時・分・秒に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] hour 現在の時
                @arg NULL : 時を返さない
                @arg その他
            @param [out] minute 現在の分
                @arg NULL : 分を返さない
                @arg その他
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @return 午前０時からの経過秒
         */
                int                     getTime                         (int* hour, int* minute, int* second) const;
        /*!
            引数に指定した分・秒をもとにして時刻部分を設定します。
            各桁で繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] minute 設定する分
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                setTime                         (int minute, int second);
        /*!
            時刻部分を分・秒に分解して取得します。
            各桁を取得する必要がない場合にはそれぞれの引数に NULL を指定することができます。
            @param [out] minute 現在の分
                @arg NULL : 分を返さない
                @arg その他
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @return 午前０時からの経過秒
         */
                int                     getTime                         (int* minute, int* second) const;
        /*!
            引数に指定した秒をもとにして時刻部分を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] second 設定する秒
            @return 自分への参照
         */
                IRXTime&                setTime                         (int second);
        /*!
            時刻部分を秒に分解して取得します。
            取得する必要がない場合には引数に NULL を指定することができます。
            @param [out] second 現在の秒
                @arg NULL : 秒を返さない
                @arg その他
            @return 午前０時からの経過秒
         */
                int                     getTime                         (int* second) const;
        /*!
            年を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setYear                         (int param);
        /*!
            年を取得します。
            @return 現在の値
         */
                int                     getYear                         (void) const;
        /*!
            月を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setMonth                        (int param);
        /*!
            月を取得します。
            @return 現在の値
         */
                int                     getMonth                        (void) const;
        /*!
            日を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setDay                          (int param);
        /*!
            日を取得します。
            @return 現在の値
         */
                int                     getDay                          (void) const;
        /*!
            時を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setHour                         (int param);
        /*!
            時を取得します。
            @return 現在の値
         */
                int                     getHour                         (void) const;
        /*!
            分を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setMinute                       (int param);
        /*!
            分を取得します。
            @return 現在の値
         */
                int                     getMinute                       (void) const;
        /*!
            秒を設定します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 設定する値
            @return 自分への参照
         */
                IRXTime&                setSecond                       (int param);
        /*!
            秒を取得します。
            @return 現在の値
         */
                int                     getSecond                       (void) const;
        /*!
            曜日を取得します。
            @return 現在の値
         */
                DayOfWeekEnum           getDayOfWeek                    (void) const;
        /*!
            時刻差を加算します。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                add                             (IRXTimeDiff const& param);
        /*!
            年を加算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                addYear                         (int param);
        /*!
            月を加算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                addMonth                        (int param);
        /*!
            日を加算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                addDay                          (int param);
        /*!
            時を加算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                addHour                         (int param);
        /*!
            分を加算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                addMinute                       (int param);
        /*!
            秒を加算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 加算する値
            @return 自分への参照
         */
                IRXTime&                addSecond                       (int param);
        /*!
            時刻差を減算します。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                sub                             (IRXTimeDiff const& param);
        /*!
            年を減算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                subYear                         (int param);
        /*!
            月を減算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                subMonth                        (int param);
        /*!
            日を減算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                subDay                          (int param);
        /*!
            時を減算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                subHour                         (int param);
        /*!
            分を減算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                subMinute                       (int param);
        /*!
            秒を減算します。
            繰り上げや繰り下げが発生したときにも正しく設定されます。
            @param [in] param 減算する値
            @return 自分への参照
         */
                IRXTime&                subSecond                       (int param);
        /*!
            値を比較します。
            @param [in] param 比較する値
            @return 比較結果
            @retval true 同じ
            @retval false 同じでない
         */
                bool                    equals                          (IRXTime const& param) const;
        /*!
            日付部分を比較します。
            @param [in] param 比較する値
            @return 比較結果
            @retval true 同じ
            @retval false 同じでない
         */
                bool                    equalsDate                      (IRXTime const& param) const;
        /*!
            時刻部分を比較します。
            @param [in] param 比較する値
            @return 比較結果
            @retval true 同じ
            @retval false 同じでない
         */
                bool                    equalsTime                      (IRXTime const& param) const;
        /*!
            値の大小を比較します。
            @param [in] param 比較する値
            @return 比較結果
            @retval 0 同じ (this == param)
            @retval >0 大きい (this > param)
            @retval <0 小さい (this < param)
         */
                int                     compare                         (IRXTime const& param) const;
        /*!
            日付部分の大小を比較します。
            @param [in] param 比較する値
            @return 比較結果
            @retval 0 同じ (this == param)
            @retval >0 大きい (this > param)
            @retval <0 小さい (this < param)
         */
                int                     compareDate                     (IRXTime const& param) const;
        /*!
            時刻部分の大小を比較します。
            @param [in] param 比較する値
            @return 比較結果
            @retval 0 同じ (this == param)
            @retval >0 大きい (this > param)
            @retval <0 小さい (this < param)
         */
                int                     compareTime                     (IRXTime const& param) const;
        /*!
            引数に指定された時刻との時刻差を取得します。
            @param [in] param 減算する値
            @return 時刻差
         */
                IRXTimeDiff             difference                      (IRXTime const& param) const;
        /*!
            引数に指定された書式に従って文字列から値を設定します。
         
            使い方１：「年/月/日 (曜日) 時:分:秒」の書式に従って文字列から読み込む
            @code
            string str;
            IRXTime time;
            IRXTime::ErrorEnum error;
         
            str = "2012/11/07 (WED) 14:48:45";
            error = time.parse("%YYYY/%MM/%DD (%WEK) %hh:%mm:%ss", str);
            if (error == IRXTime::ERROR_OK) {
                cout << "time = " << time.format("%YYYY/%MM/%DD (%WEK) %hh:%mm:%ss") << endl;
            }
            else {
                cout << "error = " << error << endl;
            }
            @endcode
            使い方２：柔軟な文字列の読み込み
            @code
            string str;
            IRXTime time;
            IRXTime::ErrorEnum error;
         
            str = "today  is 2012/11/7    14:48:45 WED";
            error = time.parse("%***** is %YYYY/%M/%D %hh:%mm:%ss", str);
            if (error == IRXTime::ERROR_OK) {
                cout << "time = " << time.formatYMD() << endl;
            }
            else {
                cout << "error = " << error << endl;
            }
            @endcode
            使い方３：時刻を表す文字列の変換
            @code
            string str;
            IRXTime time;
            IRXTime::ErrorEnum error;
         
            str = "NOV 7/2012 (WED) - 14:48:45";
            error = time.parse("%MTH %D/%Y (%***) - %h:%m:%s", str);
            if (error == IRXTime::ERROR_OK) {
                str = time.format("%YYYY/%MM/%DD (%WEEK) %hh:%mm:%ss");
                cout << "time = " << str << endl;
            }
            else {
                cout << "error = " << error << endl;
            }
            @endcode
            @param [in] format 書式を表す文字列
                @arg @%YYYY : ４桁の年 (0000 ~ 9999)
                @arg @%YY : ２桁の年 (00 ~ 99)
                @arg @%Y : 年 (0 ~ 9999)
                @arg @%MONTH : 大文字の英語の月 (JANUARY ~ DECEMBER)
                @arg @%Month : 英語の月 (January ~ December)
                @arg @%month : 小文字の英語の月 (january ~ december)
                @arg @%MTH : 大文字の英語の月の略称 (JAN ~ DEC)
                @arg @%Mth : 英語の月の略称 (Jan ~ Dec)
                @arg @%mth : 小文字の英語の月の略称 (jan ~ dec)
                @arg @%MM : ２桁の月 (01 ~ 12)
                @arg @%M : 月 (1 ~ 12)
                @arg @%DDD : ３桁の日 (001 ~ 366)
                @arg @%DD : ２桁の日 (01 ~ 31)
                @arg @%D : 日 (1 ~ 31)
                @arg @%WEEK : 大文字の英語の曜日 (SUNDAY ~ SATURDAY)
                @arg @%Week : 英語の曜日 (Sunday ~ Saturday)
                @arg @%week : 小文字の英語の曜日 (sunday ~ saturday)
                @arg @%WEK : 大文字の英語の曜日の略称 (SUN ~ SAT)
                @arg @%Wek : 英語の曜日の略称 (Sun ~ Sat)
                @arg @%wek : 小文字の英語の曜日の略称 (sun ~ sat)
                @arg @%JY : 日本語の曜日 (日 ~ 土)
                @arg @%AN : 大文字の英語の午前午後 (AM, PM)
                @arg @%an : 小文字の英語の午前午後 (am, pm)
                @arg @%AD : 大文字の英語の午前午後 (A.M., P.M.)
                @arg @%ad : 小文字の英語の午前午後 (a.m., p.m.)
                @arg @%JG : 日本語の午前午後 (午前, 午後)
                @arg @%HH : ２桁の１２時間表記の時 (01 ~ 12)
                @arg @%H : １２時間表記の時 (1 ~ 12)
                @arg @%hh : ２桁の時 (00 ~ 23)
                @arg @%h : 時 (0 ~ 23)
                @arg @%mm : ２桁の分 (00 ~ 59)
                @arg @%m : 分 (0 ~ 59)
                @arg @%ss : ２桁の秒 (00 ~ 59)
                @arg @%s : 秒 (0 ~ 59)
                @arg @%p : POSIX 時刻
                @arg @%@<space@> : % で始めた書式の終了
                @arg @%@% : % を読み込み
                @arg @%*(*...) : * の数だけ任意の文字を読み込み
                @arg @%@<その他@> : 何もしない (予約された書式)
                @arg @% 以外の文字 : 一致する文字を読み込み
            @param [in] string 解析する文字列
            @return エラー情報
            @retval IRXTime::ERROR_OK 正常終了
            @retval IRXTime::ERROR_INVALID_FORMAT フォーマットが不正
         */
                ErrorEnum               parse                           (std::string const& format, std::string const& string);
        /*!
            YYYY/MM/DD 形式の書式に従って文字列から値を設定します。
            @param [in] string 解析する文字列
            @param [in] delimiter 日付の区切り文字
            @return エラー情報
            @retval IRXTime::ERROR_OK 正常終了
            @retval IRXTime::ERROR_INVALID_FORMAT フォーマットが不正
         */
                ErrorEnum               parseYMD                        (std::string const& string, char delimiter = '/');
        /*!
            MM/DD/YYYY 形式の書式に従って文字列から値を設定します。
            @param [in] string 解析する文字列
            @param [in] delimiter 日付の区切り文字
            @return エラー情報
            @retval IRXTime::ERROR_OK 正常終了
            @retval IRXTime::ERROR_INVALID_FORMAT フォーマットが不正
         */
                ErrorEnum               parseMDY                        (std::string const& string, char delimiter = '/');
        /*!
            DD/MM/YYYY 形式の書式に従って文字列から値を設定します。
            @param [in] string 解析する文字列
            @param [in] delimiter 日付の区切り文字
            @return エラー情報
            @retval IRXTime::ERROR_OK 正常終了
            @retval IRXTime::ERROR_INVALID_FORMAT フォーマットが不正
         */
                ErrorEnum               parseDMY                        (std::string const& string, char delimiter = '/');
        /*!
            ISO8601 (YYYYMMDDThhmmss, YYYY-MM-DDThh:mm:ss) 形式の書式に従って文字列から値を設定します。
            @param [in] string 解析する文字列
            @param [in] extended 日付の区切り文字を使用するかどうか
            @return エラー情報
            @retval IRXTime::ERROR_OK 正常終了
            @retval IRXTime::ERROR_INVALID_FORMAT フォーマットが不正
         */
                ErrorEnum               parseISO8601                    (std::string const& string, bool extended = false);
        /*!
            引数に指定された書式に従って値を文字列に変換します。
         
            使い方１：「年/月/日 (曜日) 時:分:秒」の書式に従って文字列を書き出す
            @code
            IRXTime time;
            string str;
         
            time = IRXTime::currentTime();
            str = time.format("%YYYY/%MM/%DD (%WEK) %hh:%mm:%ss");
            cout << str << endl;
            @endcode
            使い方２：時刻を表す文字列の変換
            @code
            string str;
            IRXTime time;
            IRXTime::ErrorEnum error;
         
            str = "NOV 7/2012 (WED) - 14:48:45";
            error = time.parse("%MTH %D/%Y (%***) - %h:%m:%s", str);
            if (error == IRXTime::ERROR_OK) {
                str = time.format("%YYYY/%MM/%DD (%WEEK) %hh:%mm:%ss");
                cout << "time = " << str << endl;
            }
            else {
                cout << "error = " << error << endl;
            }
            @endcode
            @param [in] format 書式を表す文字列
                @arg @%YYYY : ４桁の年 (0000 ~ 9999)
                @arg @%YY : ２桁の年 (00 ~ 99)
                @arg @%Y : 年 (0 ~ 9999)
                @arg @%MONTH : 大文字の英語の月 (JANUARY ~ DECEMBER)
                @arg @%Month : 英語の月 (January ~ December)
                @arg @%month : 小文字の英語の月 (january ~ december)
                @arg @%MTH : 大文字の英語の月の略称 (JAN ~ DEC)
                @arg @%Mth : 英語の月の略称 (Jan ~ Dec)
                @arg @%mth : 小文字の英語の月の略称 (jan ~ dec)
                @arg @%MM : ２桁の月 (01 ~ 12)
                @arg @%M : 月 (1 ~ 12)
                @arg @%DDD : ３桁の日 (001 ~ 366)
                @arg @%DD : ２桁の日 (01 ~ 31)
                @arg @%D : 日 (1 ~ 31)
                @arg @%WEEK : 大文字の英語の曜日 (SUNDAY ~ SATURDAY)
                @arg @%Week : 英語の曜日 (Sunday ~ Saturday)
                @arg @%week : 小文字の英語の曜日 (sunday ~ saturday)
                @arg @%WEK : 大文字の英語の曜日の略称 (SUN ~ SAT)
                @arg @%Wek : 英語の曜日の略称 (Sun ~ Sat)
                @arg @%wek : 小文字の英語の曜日の略称 (sun ~ sat)
                @arg @%JY : 日本語の曜日 (日 ~ 土)
                @arg @%AN : 大文字の英語の午前午後 (AM, PM)
                @arg @%an : 小文字の英語の午前午後 (am, pm)
                @arg @%AD : 大文字の英語の午前午後 (A.M., P.M.)
                @arg @%ad : 小文字の英語の午前午後 (a.m., p.m.)
                @arg @%JG : 日本語の午前午後 (午前, 午後)
                @arg @%HH : ２桁の１２時間表記の時 (01 ~ 12)
                @arg @%H : １２時間表記の時 (1 ~ 12)
                @arg @%hh : ２桁の時 (00 ~ 23)
                @arg @%h : 時 (0 ~ 23)
                @arg @%mm : ２桁の分 (00 ~ 59)
                @arg @%m : 分 (0 ~ 59)
                @arg @%ss : ２桁の秒 (00 ~ 59)
                @arg @%s : 秒 (0 ~ 59)
                @arg @%p : POSIX 時刻
                @arg @%@<space@> : % で始めた書式の終了
                @arg @%@% : % を書き出し
                @arg @%* : * を書き出し
                @arg @%@<その他@> : 何もしない (予約された書式)
                @arg @% 以外の文字 : そのまま書き出し
            @return 結果の文字列
         */
                std::string             format                          (std::string const& format) const;
        /*!
            YYYY/MM/DD 形式の書式に従って値を文字列に変換します。
            @param [in] delimiter 日付の区切り文字
            @return 結果の文字列
         */
                std::string             formatYMD                       (char delimiter = '/') const;
        /*!
            MM/DD/YYYY 形式の書式に従って値を文字列に変換します。
            @param [in] delimiter 日付の区切り文字
            @return 結果の文字列
         */
                std::string             formatMDY                       (char delimiter = '/') const;
        /*!
            DD/MM/YYYY 形式の書式に従って値を文字列に変換します。
            @param [in] delimiter 日付の区切り文字
            @return 結果の文字列
         */
                std::string             formatDMY                       (char delimiter = '/') const;
        /*!
            ISO8601 (YYYYMMDDThhmmss, YYYY-MM-DDThh:mm:ss) 形式の書式に従って値を文字列に変換します。
            @param [in] extended 日付の区切り文字を使用するかどうか
            @return 結果の文字列
         */
                std::string             formatISO8601                   (bool extended = false) const;
        /*!
            POSIX 時刻として値を取得します。
            @return POSIX 時刻
         */
                time_t                  asTime_t                        (void) const;
        /*!
            基準時刻を取得します。
            @return 基準時刻
         */
        static  IRXTime                 epochTime                       (void);
        /*!
            基準時刻を UTC 時間で取得します。
            @return 基準時刻
         */
        static  IRXTime                 epochUTCTime                    (void);
        /*!
            現在時刻を取得します。
            @return 現在時刻
         */
        static  IRXTime                 currentTime                     (void);
        /*!
            現在時刻を UTC 時間で取得します。
            @return 現在時刻
         */
        static  IRXTime                 currentUTCTime                  (void);
    private:
                ErrorEnum               from                            (int year, int month, int day, int hour, int minute, int second, bool truncate);
                ErrorEnum               from                            (time_t utc, bool local);
                time_t                  to                              (void) const;
        static  SearchEnum              search                          (std::string const& string, int* index, char* key);
        static  SearchEnum              search                          (std::string const& string, int* index, char* key, int* hint);
        static  void                    step                            (std::string const& string, int* index, char* key);
        static  bool                    compare                         (std::string const& string, int* index, char const* compare);
};

/*!
    時刻と時刻差を加算します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 時刻
 */
extern  IRXTime                         operator+                       (IRXTime const& x, IRXTimeDiff const& y);
/*!
    時刻差と時刻を加算します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 時刻
 */
extern  IRXTime                         operator+                       (IRXTimeDiff const& x, IRXTime const& y);
/*!
    時刻から時刻差を減算します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 時刻
 */
extern  IRXTime                         operator-                       (IRXTime const& x, IRXTimeDiff const& y);
/*!
    時刻から時刻を減算します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 時刻差
 */
extern  IRXTimeDiff                     operator-                       (IRXTime const& x, IRXTime const& y);
/*!
    値を比較します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 比較結果
    @retval true 同じ
    @retval false 同じでない
 */
extern  bool                            operator==                      (IRXTime const& x, IRXTime const& y);
/*!
    値を逆比較します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 比較結果
    @retval true 同じでない
    @retval false 同じ
 */
extern  bool                            operator!=                      (IRXTime const& x, IRXTime const& y);
/*!
    値の大小（未満）を比較します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 比較結果
    @retval true 真 (this < param)
    @retval false 偽 (this >= param)
 */
extern  bool                            operator<                       (IRXTime const& x, IRXTime const& y);
/*!
    値の大小（以下）を比較します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 比較結果
    @retval true 真 (this <= param)
    @retval false 偽 (this > param)
 */
extern  bool                            operator<=                      (IRXTime const& x, IRXTime const& y);
/*!
    値の大小（より大きい）を比較します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 比較結果
    @retval true 真 (this > param)
    @retval false 偽 (this <= param)
 */
extern  bool                            operator>                       (IRXTime const& x, IRXTime const& y);
/*!
    値の大小（以上）を比較します。
    @param [in] x 左辺値
    @param [in] y 右辺値
    @return 比較結果
    @retval true 真 (this >= param)
    @retval false 偽 (this < param)
 */
extern  bool                            operator>=                      (IRXTime const& x, IRXTime const& y);

/*public */inline IRXTime::IRXTime(void)
{
    from(0, false);
}

/*public */inline IRXTime::IRXTime(IRXTime const& param)
{
    set(param);
}

/*public */inline IRXTime::IRXTime(int year, int month, int day, int hour, int minute, int second)
{
    set(year, month, day, hour, minute, second);
}

/*public */inline IRXTime::IRXTime(int year, int day, int hour, int minute, int second)
{
    set(year, day, hour, minute, second);
}

/*public */inline IRXTime::IRXTime(int year, int hour, int minute, int second)
{
    set(year, hour, minute, second);
}

/*public */inline IRXTime::IRXTime(int year, int minute, int second)
{
    set(year, minute, second);
}

/*public */inline IRXTime::IRXTime(int year, int second)
{
    set(year, second);
}

/*public */inline IRXTime::IRXTime(time_t param)
{
    set(param);
}

/*public */inline IRXTime::~IRXTime(void)
{
}

/*public */inline IRXTime& IRXTime::operator=(IRXTime const& param)
{
    return set(param);
}

/*public */inline IRXTime& IRXTime::operator+=(IRXTimeDiff const& param)
{
    return add(param);
}

/*public */inline IRXTime& IRXTime::operator-=(IRXTimeDiff const& param)
{
    return sub(param);
}

/*public */inline int IRXTime::getYear(void) const
{
    return _year;
}

/*public */inline int IRXTime::getMonth(void) const
{
    return _month;
}

/*public */inline int IRXTime::getDay(void) const
{
    return _day;
}

/*public */inline int IRXTime::getHour(void) const
{
    return _hour;
}

/*public */inline int IRXTime::getMinute(void) const
{
    return _minute;
}

/*public */inline int IRXTime::getSecond(void) const
{
    return _second;
}

/*public */inline IRXTime::DayOfWeekEnum IRXTime::getDayOfWeek(void) const
{
    return _week;
}

/*public */inline time_t IRXTime::asTime_t(void) const
{
    return to();
}

inline IRXTime operator+(IRXTime const& x, IRXTimeDiff const& y)
{
    return IRXTime(x).operator+=(y);
}

inline IRXTime operator+(IRXTimeDiff const& x, IRXTime const& y)
{
    return IRXTime(y).operator+=(x);
}

inline IRXTime operator-(IRXTime const& x, IRXTimeDiff const& y)
{
    return IRXTime(x).operator-=(y);
}

inline IRXTimeDiff operator-(IRXTime const& x, IRXTime const& y)
{
    return x.difference(y);
}

inline bool operator==(IRXTime const& x, IRXTime const& y)
{
    return x.equals(y);
}

inline bool operator!=(IRXTime const& x, IRXTime const& y)
{
    return !x.equals(y);
}

inline bool operator<(IRXTime const& x, IRXTime const& y)
{
    return (x.compare(y) < 0);
}

inline bool operator<=(IRXTime const& x, IRXTime const& y)
{
    return (x.compare(y) <= 0);
}

inline bool operator>(IRXTime const& x, IRXTime const& y)
{
    return (x.compare(y) > 0);
}

inline bool operator>=(IRXTime const& x, IRXTime const& y)
{
    return (x.compare(y) >= 0);
}

}// end of namespace

#endif
