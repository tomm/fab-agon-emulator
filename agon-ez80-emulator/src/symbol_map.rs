use std::collections::HashMap;

pub fn read_zds_map_file(filename: &str) -> Result<HashMap<String, u32>, String> {

    match std::fs::read_to_string(filename) {
        Ok(contents) => read_zds_map(&contents),
        Err(e) => Err(format!("IO error reading symbol map '{}': {}", filename, e.to_string()))
    }
}

pub fn read_zds_map(contents: &str) -> Result<HashMap<String, u32>, String> {
    let mut map = HashMap::new();

    let mut lines = contents.lines().skip_while(|l| *l != "EXTERNAL DEFINITIONS:");
    lines.next();//.expect("EXTERNAL DEFINITIONS:");
    lines.next();//.expect("=====================");
    lines.next();//.expect("");
    lines.next();//.expect("Symbol                               Address Module          Segment");
    lines.next();//.expect("-------------------------------- ----------- --------------- --------------------------------");


    for line in lines {
        if line == "" { break }
        let mut words = line.split_whitespace();

        let sym = words.next().ok_or("failed to parse symbol")?;
        // in form 'C:1234ab' or 'D:1234ab'
        let address = words.next().ok_or("failed to parse symbol address")?.get(2..).ok_or("failed to parse symbol address")?;

        match u32::from_str_radix(address, 16) {
            Ok(addr) => { map.insert(sym.to_string(), addr); }
            Err(_) => {return Err(format!("failed to parse symbol address in line: {}", line))}
        }
    }

    Ok(map)
}

#[test]
fn test_read_zds_map_file() {
    println!("{:?}", read_zds_map_file("MOS.map"));
}
