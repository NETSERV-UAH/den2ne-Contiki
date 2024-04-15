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
    if abs(arrow.getP1().getX() - arrow.getP2().getX()) > abs(arrow.getP1().getY() - arrow.getP2().getY()):
        if arrow.getP1().getX() - arrow.getP2().getX() > 0:                             #FROM LEFT TO RIGHT
            desfaseX = 0
            desfaseY = -15
        else:                                                                           #FROM LEFT TO RIGHT
            desfaseX = 0
            desfaseY = +15
    else:
        if arrow.getP1().getY() - arrow.getP2().getY() > 0:                             #UPWARDS
            if (message['Type'] == "SetHLMAC"):
                desfaseX = 60
                desfaseY = 15
            else:
                desfaseX = 20
                desfaseY = 0
        else:                                                                           #DOWNWARDS
            if (message['Type'] == "SetHLMAC"):
                desfaseX = -60
                desfaseY = -15
            else:
                desfaseX = -20
                desfaseY = 0
    if (message['Type'] == "Hello"):
        return Text(Point(arrow.getCenter().getX()-abs(desfaseX), arrow.getCenter().getY()-abs(desfaseY)), message['Type'])
    elif (message['Type'] == "SetHLMAC"):
        return Text(Point(arrow.getCenter().getX()+desfaseX, arrow.getCenter().getY()+desfaseY), message['Type'] + ": " + message['Content']['Prefix'] + '.' + message['Content']['HLMAC'][mac])
    elif (message['Type'] == "Load"):
        return Text(Point(arrow.getCenter().getX()+desfaseX, arrow.getCenter().getY()+desfaseY), message['Type'] + ": " + message["Value"])
    elif (message['Type'] == "Share"):
        return Text(Point(arrow.getCenter().getX()+desfaseX, arrow.getCenter().getY()+desfaseY), message['Type'] + ": " + message["Value"])

def show_info(win, num_nodes):
    info = []
    table_rec = np.empty((num_nodes, 3), dtype=Rectangle)
    table_text = np.empty((num_nodes, 3), dtype=Text)
    Rectangle(Point(70, 20), Point(230, 40)).draw(win).setFill("light blue")
    Text(Point(150, 32), "HLMAC").draw(win)
    Rectangle(Point(230, 20), Point(310, 40)).draw(win).setFill("light blue")
    Text(Point(270, 32), "LOAD").draw(win)
    Rectangle(Point(310, 20), Point(390, 40)).draw(win).setFill("light blue")
    Text(Point(350, 32), "SHARE").draw(win)

    for i in range(num_nodes):
        Rectangle(Point(10, 40+25*i), Point(70, 40+25*(i+1))).draw(win).setFill("light blue")
        info.append(Text(Point(40, 30+25*(i+1)), "Node " + str(i+1)))
        info[i].draw(win)
        table_rec[i][0] = Rectangle(Point(70,  40+25*i), Point(230, 40+25*(i+1)))
        table_rec[i][1] = Rectangle(Point(230, 40+25*i), Point(310, 40+25*(i+1)))
        table_rec[i][2] = Rectangle(Point(310, 40+25*i), Point(390, 40+25*(i+1)))
        table_text[i][0] = Text(Point(150, 55+25*i), "-")
        table_text[i][1] = Text(Point(270, 55+25*i), "-")
        table_text[i][2] = Text(Point(350, 55+25*i), "-")
        for c in range(3):
            table_rec[i][c].draw(win)
            table_text[i][c].draw(win)
    return table_text

def draw_nodes(win, dictionary):
    for val in dictionary.values():
        val[0].draw(win)
        val[1].draw(win)

def erase_node(circle, text):
    if type(circle) == Text:
        circle.undraw()
        text.undraw()

def node_conn(win, data, time, dictionary):

    #NUMBER OF STEPS IN TOPOLOGY
    len_hlmac_ini = 2
    len_hlmac_step = 3
    max_step = int(0)
    for k in range(len(data['Node'])):
        for c in range(len(data['Node'][k]['Message'])):
            if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and data['Node'][k]['Message'][c]['Timestamp'] == str(time):
                if int((len(data['Node'][k]['Message'][c]['Value']) - len_hlmac_ini)/len_hlmac_step + 1) > max_step:
                    max_step = int((len(data['Node'][k]['Message'][c]['Value']) - len_hlmac_ini)/len_hlmac_step + 1)

    node = np.empty(len(data['Node']), dtype=Circle)
    node_text = np.empty(len(data['Node']), dtype=Text)
    node_index = np.empty(len(data['Node']), dtype=int)
    hlmac_len_list = [[] for i in range(max_step)]

    added_nodes = []
    hlmac_list = [[] for i in range(max_step)]
    for l in range(max_step):
        if l == 0:
            for k in range(len(data['Node'])):
                for c in range(len(data['Node'][k]['Message'])):
                    if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and data['Node'][k]['Message'][c]['Timestamp'] == str(time) and len(data['Node'][k]['Message'][c]['Value']) == len_hlmac_ini + len_hlmac_step * l and not k in added_nodes:
                        hlmac_len_list[l].append(k)
                        hlmac_list[l].append(data['Node'][k]['Message'][c]['Value'])
                        added_nodes.append(k)
        else:
            for i in range(len(hlmac_len_list[l-1])):
                for k in range(len(data['Node'])):
                    for c in range(len(data['Node'][k]['Message'])):
                        if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and data['Node'][k]['Message'][c]['Timestamp'] == str(time) and len(data['Node'][k]['Message'][c]['Value']) == len_hlmac_ini + len_hlmac_step * l and not k in added_nodes and hlmac_list[l-1][i] in data['Node'][k]['Message'][c]['Value']:
                            hlmac_len_list[l].append(k)
                            hlmac_list[l].append(data['Node'][k]['Message'][c]['Value'])
                            added_nodes.append(k)

    #ASSIGNS POSITION IN DRAWING ACCORDING TO HLMAC LENGTH
    index = 0
    for c in range(len(hlmac_len_list)):
        if c == 0: #or len(hlmac_len_list[c]) <= 2:
            y_offset = 0
        else:
            y_offset = -35
        x_offset = ((len(hlmac_len_list[c]) / 2) - 1) * 200
        for k in range(len(hlmac_len_list[c])):
            node_index[index] = hlmac_len_list[c][k]
            #dict_circle.update({data['Node'][hlmac_len_list[c][k]]['MAC']: Circle(Point(650-x_offset,50+y_offset+220*c), 20)})
            node[hlmac_len_list[c][k]] = Circle(Point(650-x_offset,50+y_offset+220*c), 20)
            index += 1
            while not index-1 in added_nodes and index <= len(data['Node']):
                index += 1
            x_offset -= 200
            y_offset = -y_offset
            #dict_text.update({data['Node'][hlmac_len_list[c][k]]['MAC']: Circle(Point(650-x_offset,50+y_offset+220*c), 20)})
            node_text[hlmac_len_list[c][k]] = Text(node[hlmac_len_list[c][k]].getCenter(), str(index))
            if not data['Node'][hlmac_len_list[c][k]]['MAC'] in dictionary:
                dictionary[data['Node'][hlmac_len_list[c][k]]['MAC']] = [Circle(Point(650-x_offset,50+y_offset+220*c), 20), Text(Circle(Point(650-x_offset,50+y_offset+220*c), 20).getCenter(), str(len(dictionary.keys())+1))]
    breakpoint()  
    return node, node_text, node_index, dictionary
    

def main():
    tipos_mensaje = ["Hello", "SetHLMAC", "Load", "Share"]
    win = GraphWin("First Escenary", 1300, 900)
    win.setBackground("white")
    # draw_next(win)

    #data = json_file("prueba_demo_4_2.json")
    data = json_file("timers/test_demo_7.json")
    #data = json_file("prueba_demo_4_mesh.json")


    # node = []
    # node.append(Circle(Point(700,100), 20))
    # node.append(Circle(Point(500,350), 20))
    # node.append(Circle(Point(900,350), 20))

    #node.append(draw_node(win, Point(700,100), len(node)+1))
    #node.append(draw_node(win, Point(400,350), len(node)+1))
    #node.append(draw_node(win, Point(1000,350), len(node)+1))
    #node.append(draw_node(win, Point(700,600), len(node)+1))

    #node.append(Circle(Point(700,100), 20))
    #node.append(Circle(Point(400,350), 20))
    #node.append(Circle(Point(1000,350), 20))
    #node.append(Circle(Point(700,600), 20))

    #node.append(Circle(Point(700,100), 20))
    #node.append(Circle(Point(700,300), 20))
    #node.append(Circle(Point(500,550), 20))
    #node.append(Circle(Point(900,550), 20))
    #node.append(Circle(Point(300,750), 20))
    #node.append(Circle(Point(1100,550), 20))
    #node.append(Circle(Point(1000,750), 20))

    dictionary = {}
    table_text  = show_info(win, len(data['Node']))

    #for k in range(len(data['Node'])):
    #    if data['Node'][k]['Type'] == "Root":
    #        total_messages = len(data['Node'][k]['Message'])
    
    # root_index = 0

    #NUMBER OF ITERATIONS
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

    # for c in range(len(data['Node'])):
    #     if first_timestamp[c] == 0:
    #         draw_node(win, node[c], node_index[c] + 1)
    #         #breakpoint()
    # key = win.getKey()
    # while key != "n":
    #     key = win.getKey()

    last_message = np.zeros(len(data['Node']), dtype=int)
    arrow = []
    arrow_message = []
    for time in range(timestamps+1):
        # for c in range(len(data['Node'])):
        #     if first_timestamp[c] == time and time != 0:
        #         draw_node(win, node[c], node_index[c] + 1)
        #         key = win.getKey()
        #         while key != "n":
        #             key = win.getKey()

        #NODE CONTAINS EACH NODE'S CIRCLE AND NODE_INDEX ITS NUMBER
        node, node_text, node_index, dictionary = node_conn(win, data, time, dictionary)
        # list_mac = []
        # dictionary = {}
        # for i in range(len(data['Node'])):
        #     list_mac.append(data['Node'][i]['MAC'])
        #     dictionary.update({data['Node'][i]['MAC']: node[i]})

        # for i in range(len(node_text)):
        #     draw_node(win, node[i], node_text[i])
        # key = win.getKey()
        # while key != "n":
        #     key = win.getKey()

        draw_nodes(win, dictionary)
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
                            # breakpoint()
                            arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][last_message[k]]['Origin']][0], dictionary[data['Node'][k]['MAC']][0], data['Node'][k]['Message'][last_message[k]]['Type'], 0))
                            arrow[len(arrow)-1].setWidth(3)
                            arrow[len(arrow)-1].draw(win)
                            arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][last_message[k]], data['Node'][k]['MAC']))
                            arrow_message[len(arrow)-1].draw(win)
                        elif data['Node'][k]['Message'][last_message[k]]['Type'] == "INFO":
                            if data['Node'][k]['Message'][last_message[k]]['Content'] == "Edge":
                                if not data['Node'][k]['MAC'] in edge_nodes:
                                    edge_nodes.append(data['Node'][k]['MAC'])
                            elif data['Node'][k]['Message'][last_message[k]]['Content'] == "HLMAC added":
                                table_text[node_index[k]][0].undraw()
                                table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][last_message[k]]['Value'])
                                table_text[node_index[k]][0].draw(win)
                        last_message[k] += 1
                key = win.getKey()
                while key != "n":
                    key = win.getKey()
                for i in range(len(arrow)):
                    arrow[i].undraw()
                    arrow_message[i].undraw()
            elif c == "SetHLMAC":
                #breakpoint()
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
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']][0], dictionary[data['Node'][k]['MAC']][0], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":                                    
                                #breakpoint()
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added" and data['Node'][k]['MAC'] in assigned_nodes:
                                    print(k)
                                    print(node_index[k])
                                    print(table_text[node_index[k]][0])
                                    table_text[node_index[k]][0].undraw()
                                    table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                    table_text[node_index[k]][0].draw(win)
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
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']][0], dictionary[data['Node'][k]['MAC']][0], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Load' and data['Node'][k]['MAC'] in load_nodes:
                                table_text[node_index[k]][1].undraw()
                                table_text[node_index[k]][1] = Text(table_text[node_index[k]][1].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                table_text[node_index[k]][1].draw(win)
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added":
                                    table_text[node_index[k]][0].undraw()
                                    table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                    table_text[node_index[k]][0].draw(win)
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
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']][0], dictionary[data['Node'][k]['MAC']][0], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Share' and data['Node'][k]['MAC'] in share_nodes:
                                table_text[node_index[k]][2].undraw()
                                table_text[node_index[k]][2] = Text(table_text[node_index[k]][2].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                table_text[node_index[k]][2].draw(win)
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added":
                                    table_text[node_index[k]][0].undraw()
                                    table_text[node_index[k]][0] = Text(table_text[node_index[k]][0].getAnchor(), data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                    table_text[node_index[k]][0].draw(win)
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
