from graphics import *
import json
import numpy as np
import pdb

#a941 => 30 (Root)
#3163 => 98 (Common)
#0245 => 85 (Common)
#9193 => 10 (Common)

def json_file(file):
    f = open(file)
    data = json.load(f)
    f.close()
    return data

def draw_next(win):
    button = Rectangle(Point(1130,840), Point(1280,880))
    button.setFill("green")
    button.draw(win)
    text = Text(button.getCenter(), "Press 'n' to continue")
    text.draw(win)

def create_arrow(origin, destiny, mes_type, flag):                              #FLAG TO AVOID CONFUSING ARROW DIRECTIONS
    if abs(origin.getCenter().getX() - destiny.getCenter().getX()) > abs(origin.getCenter().getY() - destiny.getCenter().getY()):
        if origin.getCenter().getX() - destiny.getCenter().getX() > 0:                  #ORIGIN AT DESTINY'S LEFT
            point_origin = Point(origin.getP1().getX(),
                                 origin.getP1().getY()+origin.getRadius()-flag*4)       #LEFT
            point_destiny = Point(destiny.getP2().getX(),
                                  destiny.getP2().getY()-destiny.getRadius()-flag*4)    #RIGHT
        else:                                                                           #ORIGIN AT DESTINY'S RIGHT
            point_origin = Point(origin.getP2().getX(),
                                 origin.getP2().getY()-origin.getRadius()+flag*4)       #RIGHT
            point_destiny = Point(destiny.getP1().getX(),
                                  destiny.getP1().getY()+destiny.getRadius()+flag*4)    #LEFT
    else:
        if origin.getCenter().getY() - destiny.getCenter().getY() > 0:                  #ORIGIN BELOW DESTINY
            point_origin = Point(origin.getP1().getX()+origin.getRadius()-flag*4,
                                 origin.getP1().getY())                                 #UP
            point_destiny = Point(destiny.getP2().getX()-destiny.getRadius()-flag*4,
                                  destiny.getP2().getY())                               #DOWN
        else:                                                                           #ORIGIN ABOVE DESTINY
            point_origin = Point(origin.getP2().getX()-origin.getRadius()+flag*4,
                                 origin.getP2().getY())                                 #DOWN
            point_destiny = Point(destiny.getP1().getX()+destiny.getRadius()+flag*4,
                                  destiny.getP1().getY())                               #UP
        
    arrow = Line(point_origin, point_destiny)
    if (mes_type == "Hello"):
        arrow.setFill("red")
    elif (mes_type == "SetHLMAC"):
        arrow.setFill("green")
    elif (mes_type == "Load"):
        arrow.setFill("blue")
    elif (mes_type == "Share"):
        arrow.setFill("orange")
    arrow.setArrow("last")
    return arrow

def arrow_text(arrow, message, mac):
    if abs(arrow.getP1().getY() - arrow.getP2().getY()) > 70:
        return Text(Point(arrow.getCenter().getX()-35, arrow.getCenter().getY()), message['Type'])
    elif arrow.getP1().getX() - arrow.getP2().getX() < 200 and abs(arrow.getP1().getY() - arrow.getP2().getY()) == 70:
        return Text(Point(arrow.getCenter().getX(), arrow.getCenter().getY()+30), message['Type'])
    elif arrow.getP1().getX() - arrow.getP2().getX() > -200 and abs(arrow.getP1().getY() - arrow.getP2().getY()) == 70:
        return Text(Point(arrow.getCenter().getX(), arrow.getCenter().getY()+30), message['Type'])
    elif abs(arrow.getP1().getY() - arrow.getP2().getY()) == 0:
        return Text(Point(arrow.getCenter().getX()+60, arrow.getCenter().getY()-15), message['Type'])
    else: 
        return Text(Point(arrow.getCenter().getX(), arrow.getCenter().getY()), message['Type'])

def show_info(win, num_nodes):
    info = []
    table_rec = np.empty((num_nodes, 3), dtype=Rectangle)
    table_text = np.empty((num_nodes, 3), dtype=Text)
    rec = Rectangle(Point(110, 10), Point(280, 45))
    rec.setFill("light blue")
    rec.draw(win).setWidth(3)
    Text(Point(195, 32), "HLMAC").draw(win).setSize(20)
    rec = Rectangle(Point(280, 10), Point(370, 45))
    rec.setFill("light blue")
    rec.draw(win).setWidth(3)
    Text(Point(325, 32), "LOAD").draw(win).setSize(20)
    rec = Rectangle(Point(370, 10), Point(470, 45))
    rec.setFill("light blue")
    rec.draw(win).setWidth(3)
    Text(Point(420, 32), "SHARE").draw(win).setSize(20)

    for i in range(num_nodes):
        rec = Rectangle(Point(10, 45+40*i), Point(110, 45+40*(i+1)))
        rec.setFill("light blue")
        rec.draw(win).setWidth(3)
        info.append(Text(Point(60, 30+40*(i+1)), "Node " + str(i+1)))
        info[i].draw(win).setSize(20)
        table_rec[i][0] = Rectangle(Point(110, 45+40*i), Point(280, 45+40*(i+1)))
        table_rec[i][1] = Rectangle(Point(280, 45+40*i), Point(370, 45+40*(i+1)))
        table_rec[i][2] = Rectangle(Point(370, 45+40*i), Point(470, 45+40*(i+1)))
        table_text[i][0] = Text(Point(195, 65+40*i), "-")
        table_text[i][1] = Text(Point(325, 65+40*i), "-")
        table_text[i][2] = Text(Point(420, 65+40*i), "-")
        for c in range(3):
            table_rec[i][c].draw(win).setWidth(3)
            table_text[i][c].draw(win).setSize(20)
    return table_text

def draw_node(win, circle, num):
    text_node = Text(circle.getCenter(), str(num))
    circle.draw(win).setWidth(3)
    text_node.draw(win).setSize(25)

def node_conn(win, data):
    len_hlmac_ini = 2
    len_hlmac_step = 3
    max_step = int(0)
    for k in range(len(data['Node'])):
        for c in range(len(data['Node'][k]['Message'])):
            if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added":
                if int((len(data['Node'][k]['Message'][c]['Value']) - len_hlmac_ini)/len_hlmac_step + 1) > max_step:
                    max_step = int((len(data['Node'][k]['Message'][c]['Value']) - len_hlmac_ini)/len_hlmac_step + 1)

    node = np.empty(len(data['Node']), dtype=Circle)
    node_index = np.empty(len(data['Node']), dtype=int)
    hlmac_len_list = [[] for i in range(max_step)]

    added_nodes = []
    hlmac_list = [[] for i in range(max_step)]
    for l in range(max_step):
        if l == 0:
            for k in range(len(data['Node'])):
                for c in range(len(data['Node'][k]['Message'])):
                    if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and len(data['Node'][k]['Message'][c]['Value']) == len_hlmac_ini + len_hlmac_step * l and not k in added_nodes:
                        hlmac_len_list[l].append(k)
                        hlmac_list[l].append(data['Node'][k]['Message'][c]['Value'])
                        added_nodes.append(k)
        else:
            for i in range(len(hlmac_len_list[l-1])):
                for k in range(len(data['Node'])):
                    for c in range(len(data['Node'][k]['Message'])):
                        if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and len(data['Node'][k]['Message'][c]['Value']) == len_hlmac_ini + len_hlmac_step * l and not k in added_nodes and hlmac_list[l-1][i] in data['Node'][k]['Message'][c]['Value']: #FALTA CONDICIÓN DE HIJO
                            hlmac_len_list[l].append(k)
                            hlmac_list[l].append(data['Node'][k]['Message'][c]['Value'])
                            added_nodes.append(k)

    index = 0
    for c in range(len(hlmac_len_list)):
        if c == 0: #or len(hlmac_len_list[c]) <= 2:
            y_offset = 0
        else:
            y_offset = -35
        if len(hlmac_len_list[c]) % 2 == 0:
            x_offset = ((len(hlmac_len_list[c]) / 2) - 1) * 200 
        else:
            x_offset = ((len(hlmac_len_list[c]) / 2) - 1) * 200
        for k in range(len(hlmac_len_list[c])):
            node_index[index] = hlmac_len_list[c][k]
            index += 1
            node[hlmac_len_list[c][k]] = Circle(Point(650-x_offset,50+y_offset+220*c), 30)
            x_offset -= 200
            y_offset = -y_offset    
    return node, node_index
    

def main():
    tipos_mensaje = ["Hello", "SetHLMAC", "Load", "Share"]
    win = GraphWin("First Escenary", 1300, 900)
    win.setBackground("white")
    # draw_next(win)

    #data = json_file("prueba_demo_4_2.json")
    #data = json_file("test_demo_7.json")
    data = json_file("prueba_demo_4_mesh.json")

    node, node_index = node_conn(win, data)

    list_mac = []
    dictionary = {}
    for i in range(len(data['Node'])):
        list_mac.append(data['Node'][i]['MAC'])
        dictionary.update({data['Node'][i]['MAC']: node[i]})

    table_text  = show_info(win, len(data['Node']))

    #for k in range(len(data['Node'])):
    #    if data['Node'][k]['Type'] == "Root":
    #        total_messages = len(data['Node'][k]['Message'])
    
    # root_index = 0
    for k in range(len(data['Node'])):
        if data['Node'][k]['Type'] == "Root":
            # root_index = k
            for c in range(len(data['Node'][k]['Message'])):
                if data['Node'][k]['Message'][c]['Type'] == "SetHLMAC":
                    timestamps = int(data['Node'][k]['Message'][c]['Content']['Timestamp'])

    first_timestamp = []
    for k in range(len(data['Node'])):
        for c in range(len(data['Node'][k]['Message'])):
            if data['Node'][k]['Message'][c]['Type'] == "SetHLMAC":
                first_timestamp.append(int(data['Node'][k]['Message'][c]['Content']['Timestamp']))
                break

    for c in range(len(data['Node'])):
        if first_timestamp[c] == 0:
            draw_node(win, node[c], node_index[c] + 1)
            #breakpoint()
    key = win.getKey()
    while key != "n":
        key = win.getKey()

    last_message = np.zeros(len(data['Node']), dtype=int)
    arrow = []
    arrow_message = []
    for time in range(timestamps+1):
        for c in range(len(data['Node'])):
            if first_timestamp[c] == time and time != 0:
                draw_node(win, node[c], node_index[c] + 1)
                key = win.getKey()
                while key != "n":
                    key = win.getKey()

        edge_nodes = []
        for c in tipos_mensaje:
            if c == "Hello":
                for k in range(len(data['Node'])):
                    #breakpoint()
                    while first_timestamp[k] <= time and last_message[k] < len(data['Node'][k]['Message']) and data['Node'][k]['Message'][last_message[k]]['Type'] != "SetHLMAC": #(data['Node'][k]['Message'][last_message[k]]['Type'] == "Hello" or data['Node'][k]['Message'][last_message[k]]['Type'] == "INFO"):
                        if data['Node'][k]['MAC'] != data['Node'][k]['Message'][last_message[k]]['Origin'] and data['Node'][k]['Message'][last_message[k]]['Type'] == 'Hello':
                            arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][last_message[k]]['Origin']], dictionary[data['Node'][k]['MAC']], data['Node'][k]['Message'][last_message[k]]['Type'], 0))
                            arrow[len(arrow)-1].setWidth(3)
                            arrow[len(arrow)-1].draw(win)
                            arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][last_message[k]], data['Node'][k]['MAC']))
                            arrow_message[len(arrow)-1].draw(win).setSize(20)
                        elif data['Node'][k]['Message'][last_message[k]]['Type'] == "INFO":
                            if data['Node'][k]['Message'][last_message[k]]['Content'] == "Edge":
                                if not data['Node'][k]['MAC'] in edge_nodes:
                                    edge_nodes.append(data['Node'][k]['MAC'])
                            elif data['Node'][k]['Message'][last_message[k]]['Content'] == "HLMAC added":
                                table_text[node_index[k]][0].undraw()
                                table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][last_message[k]]['Value'])
                                table_text[node_index[k]][0].draw(win).setSize(20)
                        last_message[k] += 1
                key = win.getKey()
                while key != "n":
                    key = win.getKey()
                for i in range(len(arrow)):
                    arrow[i].undraw()
                    arrow_message[i].undraw()

            elif c == "SetHLMAC":
                prefix = 2
                sethlmac_done = 0
                while sethlmac_done == 0:
                    sethlmac_done = 1
                    aux_last_message = last_message.copy()
                    for k in range(len(data['Node'])):
                        assigned_nodes = []
                        while first_timestamp[k] <= time and aux_last_message[k] < len(data['Node'][k]['Message']) and (data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "SetHLMAC" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO"):
                            if data['Node'][k]['MAC'] != data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'SetHLMAC' and len(data['Node'][k]['Message'][aux_last_message[k]]['Content']['Prefix']) == prefix:
                                sethlmac_done = 0
                                assigned_nodes.append(data['Node'][k]['MAC'])
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']], dictionary[data['Node'][k]['MAC']], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":                                    
                                #breakpoint()
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added" and data['Node'][k]['MAC'] in assigned_nodes:
                                    table_text[node_index[k]][0].undraw()
                                    table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                    table_text[node_index[k]][0].draw(win).setSize(20)
                            aux_last_message[k] += 1
                    if sethlmac_done == 0:
                        key = win.getKey()
                        while key != "n":
                            key = win.getKey()
                    for i in range(len(arrow)):
                        arrow[i].undraw()
                        arrow_message[i].undraw()
                    prefix += 3
                last_message = aux_last_message.copy()

            elif c == "Load":
                load_nodes_prev  = edge_nodes.copy()
                load_nodes  = edge_nodes.copy()
                load_nodes_next  = []
                load_done = 0
                while load_done == 0:
                    load_done = 1
                    aux_last_message = last_message.copy()
                    for k in range(len(data['Node'])):
                        while first_timestamp[k] <= time and aux_last_message[k] < len(data['Node'][k]['Message']) and (data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Load" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Share" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO"):
                            if data['Node'][k]['MAC'] != data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Load' and data['Node'][k]['Message'][aux_last_message[k]]['Origin'] in load_nodes:
                                load_done = 0
                                if not data['Node'][k]['MAC'] in load_nodes_prev and not data['Node'][k]['MAC'] in load_nodes_next and aux_last_message[k]+1 < len(data['Node'][k]['Message']) and data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]+1]['Origin']:
                                    load_nodes_next.append(data['Node'][k]['MAC'])
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']], dictionary[data['Node'][k]['MAC']], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Load' and data['Node'][k]['MAC'] in load_nodes:
                                table_text[node_index[k]][1].undraw()
                                table_text[node_index[k]][1] = Text(table_text[node_index[k]][1].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                table_text[node_index[k]][1].draw(win).setSize(20)
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added":
                                    table_text[node_index[k]][0].undraw()
                                    table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                    table_text[node_index[k]][0].draw(win).setSize(20)
                            aux_last_message[k] += 1
                    load_nodes_prev.extend(load_nodes)
                    load_nodes = load_nodes_next.copy()
                    load_nodes_next.clear()
                    if load_done == 0:
                        key = win.getKey()
                        while key != "n":
                            key = win.getKey()
                    for i in range(len(arrow)):
                        arrow[i].undraw()
                        arrow_message[i].undraw()
                #last_message = aux_last_message.copy()

            elif c == "Share":
                share_nodes_prev  = edge_nodes.copy()
                share_nodes  = edge_nodes.copy()
                share_nodes_next  = []
                share_done = 0
                while share_done == 0:
                    share_done = 1
                    aux_last_message = last_message.copy()
                    for k in range(len(data['Node'])):
                        while first_timestamp[k] <= i and aux_last_message[k] < len(data['Node'][k]['Message']) and (data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Load" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Share" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO"):
                            if data['Node'][k]['MAC'] != data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Share' and data['Node'][k]['Message'][aux_last_message[k]]['Origin'] in share_nodes:
                                share_done = 0
                                if not data['Node'][k]['MAC'] in share_nodes_prev and not data['Node'][k]['MAC'] in share_nodes_next and aux_last_message[k]+1 < len(data['Node'][k]['Message']) and data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]+1]['Origin']:
                                    share_nodes_next.append(data['Node'][k]['MAC'])
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']], dictionary[data['Node'][k]['MAC']], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Share' and data['Node'][k]['MAC'] in share_nodes:
                                table_text[node_index[k]][2].undraw()
                                table_text[node_index[k]][2] = Text(table_text[node_index[k]][2].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                table_text[node_index[k]][2].draw(win).setSize(20)
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added":
                                    table_text[node_index[k]][0].undraw()
                                    table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                    table_text[node_index[k]][0].draw(win).setSize(20)
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Convergence":
                                    convergence_time = float("{:.2f}".format(float(data['Node'][k]['Message'][aux_last_message[k]]['Value'])/128))
                            aux_last_message[k] += 1
                    share_nodes_prev.extend(share_nodes)
                    share_nodes = share_nodes_next.copy()
                    share_nodes_next.clear()
                    if share_done == 0:
                        # if time == 0:
                        #     convergence_text = Text(Point(100, 60+25*len(node)), "Convergence time: " + str(convergence_time) + "s")
                        #     convergence_text.draw(win)
                        # else:
                        #     convergence_text.undraw()
                        #     convergence_text = Text(Point(100, 60+25*len(node)), "Convergence time: " + str(convergence_time) + "s")
                        #     convergence_text.draw(win)
                        key = win.getKey()
                        while key != "n":
                            key = win.getKey()
                    for i in range(len(arrow)):
                        arrow[i].undraw()
                        arrow_message[i].undraw()
                    table_text[node_index[0]][1].undraw()
                    table_text[node_index[0]][1] = Text(table_text[node_index[0]][1].getAnchor(), 51)
                    table_text[node_index[0]][1].draw(win).setSize(20)
                last_message = aux_last_message.copy()
                edge_nodes.clear()


    

    #c = 0
    #while c < total_messages:
    #    arrow = []
    #    arrow_message = []
    #    for i in range(len(data['Node'])):
    #        if data['Node'][i]['MAC'] != data['Node'][i]['Message'][c]['Origin']:
    #            arrow.append(create_arrow(dictionary[data['Node'][i]['Message'][c]['Origin']], dictionary[data['Node'][i]['MAC']], data['Node'][i]['Message'][c]['Type']))
    #            arrow[len(arrow)-1].draw(win)
    #            arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][i]['Message'][c], data['Node'][i]['MAC']))
    #            arrow_message[len(arrow)-1].draw(win)
    #    key = win.getKey()
    #    while key != "n" and key != "p":
    #        key = win.getKey()
    #    for i in range(len(arrow)):
    #        arrow[i].undraw()
    #        arrow_message[i].undraw()
    #    if key == "n":
    #        c = c + 1
    #    elif key == "p" and c > 0:
    #        c = c - 1
    #    arrow.clear()
    #    arrow_message.clear()

    win.close()

main()
