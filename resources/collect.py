import sqlite3, json, os

ORZMIC_SQL = "orzmic_song.db"

def areFloatsEqual(x, y, epsilon=1e-4):  
    return abs(x - y) < epsilon

class ORZMICSQL:
    def __init__(self):
        self.__orz_conn = sqlite3.connect(ORZMIC_SQL)

    def update_and_insert(self, data):
        conn = self.__orz_conn
        musicid, title = data["MusicID"], data["Title"] 
        is_exists = conn.execute(f'SELECT COUNT(*) FROM songs WHERE music_id = {musicid}').fetchall()[0][0]
        is_exists = bool(is_exists)
        if not is_exists: # 如果不存在
            print(f"{title}({musicid}) 不存在")
            chart_list = []
            for chart_value in data['Difficulties']:
                chart_list.append(chart_value)
            
            chart_insert_sql = ""
            for chart in chart_list:
                if chart is None:
                    chart_insert_sql += f""",NULL,NULL,NULL,NULL """
                else:
                    chart_insert_sql += f""","{chart['ChartDesigner'].replace('"', '""')}","{chart['Difficulty'].replace('"', '""')}",{chart['NoteCount']},{chart['Rating']} """
                    
                
            insert_sql = f'''INSERT INTO songs VALUES ({data["MusicID"]}, "{data["FileName"].replace('"', '""')}", "{data["Title"].replace('"', '""')}", {data["Lockhint"]},
{int(data["InitialUnlock"])}, {int(data["Watermark"])}, "{data["Artist"].replace('"', '""')}", "{data["CoverPainter"].replace('"', '""')}", "{data["BPMRange"].replace('"', '""')}",
{data["AudioPreviewFrom"]}, {data["AudioPreviewTo"]}{chart_insert_sql})'''.replace('\\"', '""')
            conn.execute(insert_sql)
            conn.commit()
            print(f"{title}({musicid}) 已添加至数据库")
        else: # 如果存在
            print(f"{title}({musicid}) 正在效验")
            song = conn.execute(f'SELECT * FROM songs WHERE music_id = {musicid}').fetchall()[0]
            music_id, file_name, title, lock_hint, initial_unlock, watermark, artist, cover_painter, bpm, audio_preview_from, audio_preview_to, chart_designer_easy, difficulty_easy, note_count_easy, rating_easy, chart_designer_normal, difficulty_normal, note_count_normal, rating_normal, chart_designer_hard, difficulty_hard, note_count_hard, rating_hard, chart_designer_special, difficulty_special, note_count_special, rating_special  = song[0], song[1], song[2], song[3], bool(song[4]), bool(song[5]), song[6], song[7], song[8], song[9], song[10], song[11], song[12], song[13], song[14], song[15], song[16], song[17], song[18], song[19], song[20], song[21], song[22], song[23], song[24], song[25], song[26]
            # print(music_id, file_name, title, lock_hint, initial_unlock, watermark, artist, cover_painter, bpm, audio_preview_from, audio_preview_to, chart_designer_easy, difficulty_easy, note_count_easy, rating_easy, chart_designer_normal, difficulty_normal, note_count_normal, rating_normal, chart_designer_hard, difficulty_hard, note_count_hard, rating_hard, chart_designer_special, difficulty_special, note_count_special, rating_special)
            # 修改全部
            change_sql = f"""UPDATE songs SET 
file_name = "{data["FileName"].replace('"', '""')}", 
title = "{data["Title"].replace('"', '""')}", 
lock_hint = {data["Lockhint"]}, 
initial_unlock = {int(data["InitialUnlock"])}, 
watermark = {int(data["Watermark"])}, 
artist = "{data["Artist"].replace('"', '""')}", 
cover_painter = "{data["CoverPainter"].replace('"', '""')}", 
bpm = "{data["BPMRange"].replace('"', '""')}", 
audio_preview_from = {data["AudioPreviewFrom"]}, 
audio_preview_to = {data["AudioPreviewTo"]}"""
            
            LEVELS = ["easy", "normal", "hard", "special"]
            
            for index in range(len(data['Difficulties'])):
                chart_value = data['Difficulties'][index]
                level = LEVELS[index]
                if chart_value is None:
                    change_sql += f""",chart_designer_{level} = Null
,difficulty_{level} = Null
,note_count_{level} = Null
,rating_{level} = Null"""
                    # chart_insert_sql += f""",NULL,NULL,NULL,NULL """
                else:
                    change_sql += f""",chart_designer_{level} = "{chart_value['ChartDesigner'].replace('"', '""')}"
,difficulty_{level} = "{chart_value['Difficulty'].replace('"', '""')}"
,note_count_{level} = {chart_value['NoteCount']}
,rating_{level} = {chart_value['Rating']}"""
            
            change_sql += f" WHERE music_id = {musicid};"
            flag = False
            if data["FileName"] != file_name:
                print("文件名不一致")
                flag = True
            if data["Title"] != title:
                print("标题不一致")
                flag = True
            if data["Lockhint"] != lock_hint:
                print("Lockhint不一致")
                flag = True
            if data["InitialUnlock"] != bool(initial_unlock):
                print("InitialUnlock不一致")
                flag = True
            if data["Watermark"] != bool(watermark):
                print("Watermark不一致")
                flag = True
            if data["Artist"] != artist:
                print("曲师不一致")
                flag = True
            if data["CoverPainter"] != cover_painter:
                print("曲绘不一致")
                flag = True
            if data["BPMRange"] != bpm:
                print("BPM不一致")
                flag = True
            if data["AudioPreviewFrom"] != audio_preview_from:
                print("AudioPreviewFrom不一致")
                flag = True
            if data["AudioPreviewTo"] != audio_preview_to:
                print("AudioPreviewTo不一致")
                flag = True
            for index in range(len(data['Difficulties'])):
                chart_value = data['Difficulties'][index]
                if index == 0 and chart_value is not None: # ez
                    if chart_value["ChartDesigner"] != chart_designer_easy:
                        print("EZ谱师不一致")
                        flag = True
                    if chart_value["Difficulty"] != difficulty_easy:
                        print("EZ难度不一致")
                        flag = True
                    if chart_value["NoteCount"] != note_count_easy:
                        print("EZ物量不一致")
                        flag = True
                    if not areFloatsEqual(chart_value["Rating"], rating_easy):
                        print("EZ定数不一致")
                        flag = True
                elif index == 1 and chart_value is not None: # nm
                    if chart_value["ChartDesigner"] != chart_designer_normal:
                        print("NM谱师不一致")
                        flag = True
                    if chart_value["Difficulty"] != difficulty_normal:
                        print("NM难度不一致")
                        flag = True
                    if chart_value["NoteCount"] != note_count_normal:
                        print("NM物量不一致")
                        flag = True
                    if not areFloatsEqual(chart_value["Rating"], rating_normal):
                        print("NM定数不一致")
                        flag = True
                elif index == 2 and chart_value is not None: # hd
                    if chart_value["ChartDesigner"] != chart_designer_hard:
                        print("HD谱师不一致")
                        flag = True
                    if chart_value["Difficulty"] != difficulty_hard:
                        print("HD难度不一致")
                        flag = True
                    if chart_value["NoteCount"] != note_count_hard:
                        print("HD物量不一致")
                        flag = True
                    if not areFloatsEqual(chart_value["Rating"], rating_hard):
                        print("HD定数不一致")
                        flag = True
                elif index == 3 and chart_value is not None: # sp
                    if chart_value["ChartDesigner"] != chart_designer_special:
                        print("SP谱师不一致")
                        flag = True
                    if chart_value["Difficulty"] != difficulty_special:
                        print("SP难度不一致")
                        flag = True
                    if chart_value["NoteCount"] != note_count_special:
                        print("SP物量不一致")
                        flag = True
                    if not areFloatsEqual(chart_value["Rating"], rating_special):
                        print("SP定数不一致")
                        flag = True
                    
            if flag:
                conn.execute(change_sql)
                conn.commit()
            # print(change_sql)
            print(f"{title}({musicid}) 效验完成")

orz_sql = ORZMICSQL()

def loadJson(filename : str = "songs.json"):
    with open(filename, 'r', encoding='utf-8') as f:
        return json.load(f)

if __name__ == "__main__":
    songs = loadJson()
    for song in songs:
        orz_sql.update_and_insert(song)