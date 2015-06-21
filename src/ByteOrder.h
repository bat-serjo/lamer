/*
 * byteorder.h
 *
 *  Created on: May 17, 2015
 *      Author: serj
 */

#ifndef BYTEORDER_H_
#define BYTEORDER_H_

#define swap32(x) ( (((x)>>24)&0xff) |\
					(((x)>>8) &0xff00) |\
					(((x)<<8) &0xff0000) |\
					(((x)<<24)&0xff000000) )
#define swap16(x) ( (((x)>>8)&0xff) | (((x)<<8)&0xff00) )


#define CPU_IS_LITTLE_ENDIAN

#ifdef CPU_IS_LITTLE_ENDIAN
#define cpu_to_le16(x) (x)
#define cpu_to_le32(x) (x)
#define cpu_to_be16(x) swap16(x)
#define cpu_to_be32(x) swap32(x)

#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define be16_to_cpu(x) swap16(x)
#define be32_to_cpu(x) swap32(x)
#else
#define cpu_to_le16(x) swap16(x)
#define cpu_to_le32(x) swap32(x)
#define cpu_to_be16(x) (x)
#define cpu_to_be32(x) (x)

#define le16_to_cpu(x) swap16(x)
#define le32_to_cpu(x) swap32(x)
#define be16_to_cpu(x) (x)
#define be32_to_cpu(x) (x)
#endif

#endif /* BYTEORDER_H_ */
